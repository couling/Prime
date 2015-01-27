/* 

Copyright Philip Couling
All rights reserved.

This file is not for distrebution. It should not be copied or viewed by any persons
without express permission of Philip Couling.

This is a later version of prime finding and uses my bitprime technique (hence the name).

It represents a block of numbers as bits -e ach bit represents an odd number since beyond 2
no even number can be even.  It then takes numbers smaller than the square root of block's largest 
value and successivly removes all numbers that prime is a factor of from the bit mask.

It is much quicker than prime.c because each prime is touched far fewer times and secondly because 
for each time a prime is touched it uses addition and not division.

*/

#include <errno.h>

#include <pthread.h>
#include <semaphore.h>
#include <math.h>

// For memory mapping a file
#include <fcntl.h>
#include <sys/types.h>

#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>

#include "prime_shared.h"
#include "shared.h"
#include "output.h"

//#define VERBOSE_DEBUG

#define ALLOC_UNIT 0x100000
size_t APPLY_DEBUG_MASK; // This used to be a constant, but now we use a variable set in main
#define SCAN_DEBUG_MASK 0x3FFFFF

#define WRITE_BUFFER_SIZE 0x100000

// For threading
typedef struct ThreadDescriptor {
    int threadNum;
    pthread_t threadHandle;
    sem_t writeSemaphore;
    sem_t * nextThreadWriteSemaphore;
} ThreadDescriptor;


static ThreadDescriptor * threads;



//  Functions for writing primes
typedef void (* WritePrimeFunction)(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, int file);
static void writePrimeText(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, int file);
static void writePrimeSystemBinary(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, int file );
static void writePrimeCompressedBinary(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, int file );

static WritePrimeFunction writePrime = writePrimeText;

// The single file to write to (if single file is enabled);
static int theSingleFile;


// All primes less than or sqrt(endValue)
// Generating these is what initialisation is for
static size_t primeCount;                 // Number of primes stored
static size_t primesAllocated;            // Number of primes the array can store
static Prime * primes;                    // The array of primes

// used if an odd start value has been requested
static int disallow2 = 0;

// Maps used to operate on compressed prime bitmasks
static unsigned char removeMask[] = {0xFF, 0xFE, 0xFF, 0xFD, 0xFF, 0xFB, 0xFF, 0xF7, 0xFF, 0xEF, 0xFF, 0xDF, 0xFF, 0xBF, 0xFF, 0x7F};
static unsigned char checkMask[] =  {0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};

static unsigned char * lowPrimeMap;
static size_t lowPrimeCount;
static Prime lowPrimeMapSize;
static Prime lowPrimeMapMultiplyer;
static int lowPrimeModLookup[] = {0,15,0,5,0,3,0,9,0,7,0,13,0,11,0,1};

static int  bitCount[] =            {0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
                                     3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8};

static void applyPrime(Prime prime, Prime offset, unsigned char * map, size_t mapSize) {
    // A further optimization can be made here by splitting this function into two.
    // One which starts at prime^2 and one which starts at the beginning
    // process() would be able to determine when to use the two through 
    // binary search.

    Prime value;
    prime_mul_prime(value, prime, prime);
    // prime_sub_prime(value, prime, offset);

    // This function subtracts the offset from the prime rather than adding it to the map
    // It's okay. This works.
    if (prime_lt(value, offset)) {
        // value = prime - ((offset - 1) % prime) - 1;
        prime_sub_num(value, offset, 1);
        prime_mod_prime(value, value, prime);
        prime_sub_prime(value, prime, value);
        prime_sub_num(value, value, 1);
        if (!prime_is_odd(value)) prime_add_prime(value, value, prime);
    }
    else  {
        // value = (prime ^ 2) - offset;
        prime_sub_prime(value, value, offset);
    }
    Prime stepSize;
    prime_mul_num(stepSize, prime, 2);

    Prime applyTo;
    prime_set_num(applyTo, mapSize);
    prime_mul_16(applyTo, applyTo);
    while (prime_lt(value,applyTo)) {
        Prime tmp;
        prime_div_16(tmp, value);
        map[prime_get_num(tmp)] &= removeMask[prime_get_num(value) & 0x0F];
        prime_add_prime(value, value, stepSize);
    }
}



static void addPrime(Prime value) {
    if (primeCount == primesAllocated) {
        if (verbose) {
            PrimeString valueString;
            prime_to_str(valueString, value);
            stdLog("Initialized %zd primes, reached value %s (ex)", primeCount, valueString);
        }
        primesAllocated += ALLOC_UNIT;
        primes = reallocSafe(primes, primesAllocated * sizeof(Prime));
    }
    prime_cp(primes[primeCount], value);
    ++primeCount;
}



static void populateLowPrimeMap() {
    prime_set_num(lowPrimeMapSize, 1);
    lowPrimeCount = 0;
    while (prime_le(primes[lowPrimeCount], lowPrimeMax)) {
        prime_mul_prime(lowPrimeMapSize, lowPrimeMapSize, primes[lowPrimeCount]);
        ++lowPrimeCount;
    }

    if (!lowPrimeCount) return;

    lowPrimeMap = mallocSafe(prime_get_num(lowPrimeMapSize));
    memset(lowPrimeMap, 0xFF, prime_get_num(lowPrimeMapSize));

    Prime offset;
    prime_mul_num(offset,lowPrimeMapSize,4);

    for (int i=0; i < lowPrimeCount; ++i) {
        // we dont put 0 into arg 2 to be sure of clearing 0 bit
        applyPrime(primes[i], offset, lowPrimeMap, prime_get_num(lowPrimeMapSize));
    }
    
    prime_mod_num(lowPrimeMapMultiplyer, lowPrimeMapSize, 16);
    prime_mul_num(lowPrimeMapMultiplyer, lowPrimeMapSize, lowPrimeModLookup[prime_get_num(lowPrimeMapMultiplyer)]);
    prime_add_num(lowPrimeMapMultiplyer, lowPrimeMapMultiplyer, 1);
    prime_div_num(lowPrimeMapMultiplyer, lowPrimeMapMultiplyer, 16);
    prime_mod_prime(lowPrimeMapMultiplyer, lowPrimeMapMultiplyer, lowPrimeMapSize);
    if (verbose) {
        if (lowPrimeCount > 0) {
            PrimeString maxLowPrime;
            prime_to_str(maxLowPrime, primes[lowPrimeCount-1]);
            stdLog("Using %zd low primes - max %s", lowPrimeCount, maxLowPrime);
            stdLog("lowPrimeMapSize: %zd",(size_t) prime_get_num(lowPrimeMapSize));
            /*char bString[9];
            bString[8] = 0;
            for (int i = 0; i < prime_get_num(lowPrimeMapSize); ++i) {
                for (int j = 0; j < 8; ++j) {
                    bString[j] = ((lowPrimeMap[i] & (1 << j)) ? '1' : '0' );
                }
                stdLog("%s", bString);
            }*/
        }
        else {
          stdLog("No low primes used");
        }
        PrimeString s;
        prime_to_str(s, lowPrimeMapMultiplyer);
        stdLog("lowPrimeMapMultiplyer: %s", s);
    }
}



static void initializeSelf() {
    if (!silent) {
        PrimeString startValueString;
        prime_to_str(startValueString, startValue);
        PrimeString endValueString;
        prime_to_str(endValueString, endValue);
        stdLog("Running Self initialisation for %s (inc) to %s (ex)", 
            startValueString, endValueString);
    }

    // Allocate a bitmap to represent 0 to sqrt(endValue)
    primesAllocated = ALLOC_UNIT;
    primes = mallocSafe(primesAllocated * sizeof(Prime));
    Prime maxRequired;
    prime_sqrt(maxRequired, endValue);
    if (verbose) {
        PrimeString maxRequiredString;
        prime_to_str(maxRequiredString, maxRequired);
        stdLog("Initialisation will produce every prime up to %s", maxRequiredString);
    }
    Prime pRange;
    prime_add_num(pRange, maxRequired, 15);
    prime_div_16(pRange, pRange);
    size_t range = prime_get_num(pRange);
    if (verbose) stdLog("Bitmap will contain %zd bytes", range);
    unsigned char * bitmap = mallocSafe(range * sizeof(unsigned char));
    memset(bitmap, 0xFF, range);
    
    // Evaluate all primes up to sqrt(sqrt(endValue))
    // Each prime found here needs to be applied to the map
    Prime zero;
    prime_set_num(zero, 0);
    prime_sqrt(maxRequired, maxRequired);
    prime_add_num(pRange, maxRequired, 15);
    prime_div_16(pRange, pRange);
    size_t endRange = prime_get_num(pRange);
    // The first byte is tricky so we hard code primes less than 16
    // We never represent 2 as prime (we never use even numbers)
    prime_set_num(primes[0], 3);
    applyPrime(primes[0], zero, bitmap, range);
    prime_set_num(primes[1], 5);
    applyPrime(primes[1], zero, bitmap, range);
    prime_set_num(primes[2], 7);
    applyPrime(primes[2], zero, bitmap, range);
    prime_set_num(primes[3], 11);
    applyPrime(primes[3], zero, bitmap, range);
    prime_set_num(primes[4], 13);
    applyPrime(primes[4], zero, bitmap, range);
    primeCount = 5;
    size_t i = 1;
    while (i <= endRange) {
        if (verbose && !(i & SCAN_DEBUG_MASK)) 
            stdLog("Writing primes as text %02.2f%%", 100 * ((double) i)/((double) range));
        
        if (bitmap[i]) {
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[i] & checkMask[j]) {
                    Prime value;
                    getPrimeFromMap(value, zero, i, j);
                    applyPrime(value, zero, bitmap, range);
                    addPrime(value);
                }
            }
        }
        ++i;
    }

    // Evaluate all primes from sqrt(sqrt(endValue)) up to sqrt(endValue)
    // Primes here do not need to be applied to the map
    while (i < range) {
        if (verbose && !(i & SCAN_DEBUG_MASK))
            stdLog("Writing primes %02.2f%%", 100 * ((double) i)/((double) range));
        
        if (bitmap[i]) {
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[i] & checkMask[j]) {
                    Prime value;
                    getPrimeFromMap(value, zero, i, j);
                    addPrime(value);
                }
            }
        }
        ++i;
    }

    free(bitmap);

    if (!silent) stdLog("Prime array now full with %zd primes", primeCount);
}



static void finalSelf() {
    free(primes);
    free(lowPrimeMap);
}



static void writeSafe(int file, const void * buffer, size_t size) {
    if (write(file, buffer, size) != size) exitError(1, errno, "Failed to write prime file");
}



static void writePrimeText(Prime from, Prime to, size_t range, unsigned char * bitmap, int file) {
    char writeBuffer[WRITE_BUFFER_SIZE];
    size_t remainingBuffer = WRITE_BUFFER_SIZE;
    char * bufferWritePos = writeBuffer;

    int threadNum;
    if (singleFile && threadCount > 1) {
        threadNum = (*((int*)pthread_getspecific(threadNumKey))) -1;
        sem_wait(&threads[threadNum].writeSemaphore);
    }

    Prime prime_2;
    prime_set_num(prime_2, 2);
    if (prime_le(from, prime_2) && prime_ge(to, prime_2) && !disallow2) {
        bufferWritePos[0] = '2';
        bufferWritePos[1] = '\n';
        bufferWritePos += 2;
        remainingBuffer -= 2;
    }
    size_t endRange = range -1;
    for (size_t i = 0; i < endRange; ++i) {
        if (verbose && !(i & SCAN_DEBUG_MASK)) 
            stdLog("Writing primes as text %02.2f%%", 100 * ((double) i)/((double) range));

        if (bitmap[i]) {
            if (remainingBuffer < PRIME_STRING_SIZE * bitCount[bitmap[i]]) {
                writeSafe(file, writeBuffer, WRITE_BUFFER_SIZE - remainingBuffer);
                bufferWritePos = writeBuffer;
                remainingBuffer = WRITE_BUFFER_SIZE;
            }
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[i] & checkMask[j]) {
                    Prime value;
                    getPrimeFromMap(value, from, i, j);
                    int bytesWritten = prime_to_str(bufferWritePos, value);
                    bufferWritePos[bytesWritten] = '\n';
                    bufferWritePos += bytesWritten + 1;
                    remainingBuffer -= bytesWritten + 1;
                }
            }
        }
    }

    if (bitmap[endRange]) {
       if (remainingBuffer < PRIME_STRING_SIZE * bitCount[bitmap[endRange]]) {
            writeSafe(file, writeBuffer, WRITE_BUFFER_SIZE - remainingBuffer);
            bufferWritePos = writeBuffer;
            remainingBuffer = WRITE_BUFFER_SIZE;
        }
        for (int j = 1; j < 16; j+=2) {
            if (bitmap[endRange] & checkMask[j]) {
                Prime value;
                getPrimeFromMap(value, from, endRange, j);
                if (prime_lt(value, to)) {
                    int bytesWritten = prime_to_str(bufferWritePos, value);
                    bufferWritePos[bytesWritten] = '\n';
                    bufferWritePos += bytesWritten + 1;
                    remainingBuffer -= bytesWritten + 1;
                }
            }
        }
    }

    writeSafe(file, writeBuffer, WRITE_BUFFER_SIZE - remainingBuffer);

    if (singleFile && threadCount > 1) {
        sem_post(threads[threadNum].nextThreadWriteSemaphore);
    }
}



static void writePrimeSystemBinary(Prime from, Prime to, size_t range, unsigned char * bitmap, int file) {
    Prime buffer[WRITE_BUFFER_SIZE / sizeof(Prime)];
    int count = 0;

    int threadNum;
    if (singleFile && threadCount > 1) {
        threadNum = (*((int*)pthread_getspecific(threadNumKey))) -1;
        sem_wait(&threads[threadNum].writeSemaphore);
    }

    prime_set_num(buffer[0], 2);
    if (prime_le(from, buffer[0]) && prime_ge(to, buffer[0]) && !disallow2) {
        count = 1;
    }
    size_t endRange = range -1;
    for (size_t i = 0; i < endRange; ++i) {
        if (!(i & SCAN_DEBUG_MASK)) {
            if (verbose) stdLog("Writing primes %02.2f%%", 100 * ((double) i)/((double) range));
        }
        if (bitmap[i]) {
            if (count + bitCount[bitmap[i]] > WRITE_BUFFER_SIZE / sizeof(Prime)) {
                writeSafe(file, buffer, count * sizeof(Prime));
                count = 0;
            }
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[i] & checkMask[j]) {
                    getPrimeFromMap(buffer[count], from, i, j);
                    ++count;
                }
            }
        }
    }
    if (bitmap[endRange]) {
        if (count + bitCount[bitmap[endRange]] >= WRITE_BUFFER_SIZE / sizeof(Prime)) {
            writeSafe(file, buffer, count * sizeof(Prime));
            count = 0;
        }
        for (int j = 1; j < 16; j+=2) {
            if (bitmap[endRange] & checkMask[j]) {
                getPrimeFromMap(buffer[count], from, endRange, j);
                if (prime_lt(buffer[count], to)) ++count;
            }
        }
    }
    
    writeSafe(file, buffer, count * sizeof(Prime));
    
    if (singleFile && threadCount > 1) {
        sem_post(threads[threadNum].nextThreadWriteSemaphore);
    }
}



static void writePrimeCompressedBinary(Prime from, Prime to, size_t range, unsigned char * bitmap, int file ) {
    int threadNum;
    if (singleFile && threadCount > 1) {
        threadNum = (*((int*)pthread_getspecific(threadNumKey))) -1;
        sem_wait(&threads[threadNum].writeSemaphore);
    }
    CompressedBinaryHeader header;
    memset(&header, 0, sizeof(CompressedBinaryHeader));
    snprintf(header.headerSize, sizeof(header.headerSize), "%zd", sizeof(CompressedBinaryHeader));
    snprintf(header.signature, sizeof(header.signature), "Compressed Prime Binary: 1.0");
    snprintf(header.dataBlockSize, sizeof(header.dataBlockSize), "%zd", range);
    snprintf(header.skip, sizeof(header.skip), "2");
    snprintf(header.fromToSize, sizeof(header.fromToSize), "%zd",sizeof(header.from));
    snprintf(header.comments, sizeof(header.comments), "File Created on: %s\n\nCreated by...\n%s", timeNow(), getVersion());
    
    size_t foundPrimes = 0;
    size_t textSize = 0;

    Prime tmp;
    prime_set_num(tmp, 2);
    PrimeString s;
    if (prime_eq(from, tmp)) {
        if (disallow2) {
            strcpy(s, "3");
        }
        else {
            foundPrimes = 1;
            textSize = 2;
            strcpy(s, "2");
        }
    }
    else {
        prime_to_str(s, from);
    }
    snprintf(header.from, sizeof(header.from),"%s", s);
    prime_to_str(s, to);
    snprintf(header.to, sizeof(header.to), "%s",s);
    
    // Count primes in the file
    prime_sub_num(tmp, to, 1);
    prime_to_str(s, tmp);
    size_t stringSize = strlen(s);
    size_t endRange = range -1;
    if (strlen(header.from) == stringSize) {
        // Fast
        stringSize += 1;
        for (size_t i=0; i<endRange; ++i) {
            foundPrimes += bitCount[bitmap[i]];
        }
        if (bitmap[endRange]) {
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[endRange] & checkMask[j]) {
                    Prime value;
                    getPrimeFromMap(value, from, endRange, j);
                    if (prime_lt(value, to)) ++foundPrimes;
                }
            }
        }
        textSize = foundPrimes * stringSize;
    }
    else {
        // Slow
        stringSize = 2;
        PrimeString stringMaxAtSize = "9";
        Prime maxAtSize;
        prime_set_num(maxAtSize, 9);
        for (size_t i = 0; i < endRange; ++i) {
            if (bitmap[i]) {
                for (int j = 1; j < 16; j+=2) {
                    if (bitmap[i] & checkMask[j]) {
                        Prime value;
                        getPrimeFromMap(value, from, i, j);
                        while (prime_gt(value, maxAtSize)) {
                            strcat(stringMaxAtSize,"9");
                            str_to_prime(maxAtSize, stringMaxAtSize);
                            ++stringSize;
                        }
                        ++foundPrimes;
                        textSize += stringSize;
                    }
                }
            }
        }
        if (bitmap[endRange]) {
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[endRange] & checkMask[j]) {
                    Prime value;
                    getPrimeFromMap(value, from, endRange, j);
                    if (prime_lt(value, to)) {
                        while (prime_gt(value, maxAtSize)) {
                            strcat(stringMaxAtSize,"9");
                            str_to_prime(maxAtSize, stringMaxAtSize);
                            ++stringSize;
                        }
                        ++foundPrimes;
                        textSize += stringSize;
                    }
                }
            }
        }
    }

    snprintf(header.primeCount, sizeof(header.primeCount),"%zd", foundPrimes);
    snprintf(header.textSize, sizeof(header.textSize),"%zd", textSize);
    
    writeSafe(file, &header, sizeof(CompressedBinaryHeader));
    writeSafe(file, bitmap, range);
    if (singleFile && threadCount > 1) {
        sem_post(threads[threadNum].nextThreadWriteSemaphore);
    }
}



static void process(Prime from, Prime to, int file) {
    Prime tmp;

    if (!silent) {
        PrimeString fromString;
        prime_to_str(fromString, from);
        PrimeString toString;
        prime_to_str(toString, to);
        stdLog("Running process for %s (inc) to %s (ex)", fromString, toString);
    }

    Prime prime_2;
    prime_set_num(prime_2, 2);
    if (prime_lt(from, prime_2)) {
        // Process ignores even primes
        // Process doesn't know 1 isn't a prime, so skip it!
        prime_set_num(from, 2);
    }
    else if (prime_is_odd(from)) {
        // We can only accept even numbers for start values
        prime_sub_num(from, from, 1);

        // Here we may accidently bring 2 into scope by making 3 an even number (2).
        // So we specifically ban it if this has happened
        // This is the only time we need to "disallow2"
        if (prime_eq(startValue, prime_2)) disallow2 = 1;
    }

    
    prime_sub_prime(tmp, to, from);
    size_t range = (prime_get_num(tmp) + 15) / 16;
    if (verbose) stdLog("Bitmap will contain %zd bytes", range);

    unsigned char * bitmap = mallocSafe(range);

    if (lowPrimeCount) {
        //memset(bitmap, 0xFF, range);
        // Theoretically we don't need to do two mods but we don't want to hit size limits 
        // when we multiply.  So we do a mod first to bring down the size of "from"
        prime_mod_prime(tmp, from, lowPrimeMapSize);
        prime_mul_prime(tmp, tmp, lowPrimeMapMultiplyer);
        prime_mod_prime(tmp, tmp, lowPrimeMapSize);
        size_t lowPrimeMapOffset = prime_get_num(tmp);
        size_t copySize = prime_get_num(lowPrimeMapSize); 
        size_t firstCopySize = copySize - lowPrimeMapOffset;
        char * currentMapPos = bitmap;
        size_t sizeLeft = range;
        if (verbose) {
            stdLog("Using low prime map offset %zd", (size_t)prime_get_num(tmp));
        }
        if (sizeLeft > firstCopySize) {
            memcpy(currentMapPos, lowPrimeMap + lowPrimeMapOffset, firstCopySize);
            sizeLeft -= firstCopySize;
            currentMapPos += firstCopySize;
            while (sizeLeft >= copySize) {
                memcpy(currentMapPos, lowPrimeMap, copySize);
                sizeLeft -= copySize;
                currentMapPos += copySize;
            }
            if (sizeLeft > 0) {
                memcpy(currentMapPos, lowPrimeMap, sizeLeft);
            }
        }
        else {
            memcpy(currentMapPos, lowPrimeMap + lowPrimeMapOffset, sizeLeft);
        }
    }
    else {
        memset(bitmap, 0xFF, range);
    }


    if (verbose) {
        PrimeString fromString;
        PrimeString toString;
        prime_to_str(fromString, from);
        prime_to_str(toString, to);
        stdLog("Calculating primes for range %s to %s", fromString, toString);
    }
    // start with the first prime which is not a low prime
    for (size_t i = lowPrimeCount; i < primeCount; ++i) {
        if (verbose) {
            if (!(i & APPLY_DEBUG_MASK)) {
                PrimeString primeValueString;
                prime_to_str(primeValueString, primes[i]);
                stdLog("Calculating primes %02.2f%% (%zd of %zd [%s])", 
                    100 * ((double)i) / ((double)primeCount),i+1, primeCount, primeValueString);
            }
        }
        applyPrime(primes[i], from, bitmap, range);
    }

    if (lowPrimeCount) {
        size_t currentLowPrime = lowPrimeCount;
        while (currentLowPrime > 0 && prime_ge(primes[currentLowPrime-1],from)) {
            Prime v1, v2;
            prime_sub_prime(v1, primes[currentLowPrime-1], from);
            prime_div_16(v2, v1);
            bitmap[prime_get_num(v2)] |= checkMask[prime_get_num(v1) & 0x0F];
            --currentLowPrime;
        }
    }

    writePrime(from, to, range, bitmap, file);
    
    if (verbose) {
        PrimeString fromString;
        prime_to_str(fromString, from);
        PrimeString toString;
        prime_to_str(toString, to);	
        stdLog("All primes have now been discovered between %s (inc) and %s (ex)", fromString, toString);
    }

    free(bitmap);
}



static void * processAllChunks(void * threadPt) {
    ThreadDescriptor * thread = (ThreadDescriptor*) threadPt;
    pthread_setspecific(threadNumKey, &thread->threadNum);
    if (!silent) stdLog("Thread %d started", thread->threadNum);
    Prime from, to;
    prime_cp(from, startValue);
    //Prime to = startValue - (startValue % chunkSize) + chunkSize;
    prime_mod_prime(to, startValue, chunkSize);
    prime_sub_prime(to, startValue, to);
    prime_add_prime(to, to, chunkSize);

    int chunkNum = 0;
    while (prime_lt(to, endValue)) {
        if (chunkNum % threadCount == (thread->threadNum - 1)) {
            if (singleFile) process(from, to, theSingleFile);
            else {
                int file = openFileForPrime(from, to);
                process(from, to, file);
                closeFileForPrime(file);
            }
        }
        prime_cp(from,to);
        prime_add_prime(to, to, chunkSize);
        chunkNum++;
    }
    if (prime_lt(from, endValue)) {
        if (chunkNum % threadCount == (thread->threadNum - 1)) {
            if (singleFile) process(from, endValue, theSingleFile);
            else {
                int file = openFileForPrime(from, endValue);
                process(from, endValue, file);
                closeFileForPrime(file);
            }
        }
    }
    if (!silent) stdLog("Thread %d finished", thread->threadNum);
    return NULL;
}




void runThreads() {
    threads = mallocSafe(sizeof(struct ThreadDescriptor) * threadCount);

    if (threadCount == 1) {
        if (!silent) stdLog("Running single threaded");
        threads[0].threadNum = 1;
        processAllChunks(&(threads[0]));
        threads[0].threadNum = 0;
    }
    else{
    if (!silent) stdLog("Running multithread with %d threads", threadCount);
        if (singleFile && threadCount > 1) {
            // initialise the semaphores.
            // these will be used to sequence the writes correctly.
            sem_init(&(threads[0].writeSemaphore), 0, 1);
            for (int threadNum = 1; threadNum < threadCount; ++threadNum) {
                sem_init(&(threads[threadNum].writeSemaphore), 0, 0);
                threads[threadNum-1].nextThreadWriteSemaphore = &threads[threadNum].writeSemaphore;
            }
            threads[threadCount-1].nextThreadWriteSemaphore = &threads[0].writeSemaphore;
        }

        // Run all of the threads.
        for (int threadNum = 0; threadNum < threadCount; ++threadNum) {
            threads[threadNum].threadNum = threadNum+1;
            pthread_create(&(threads[threadNum].threadHandle), NULL, processAllChunks, &(threads[threadNum]));
        }

        // Wait for the threads to complete.
        int exitStatus = 0;
        if (verbose) stdLog("All threads now requested");
        for (int threadNum = 0; threadNum < threadCount; ++threadNum) {
            int * returnValue;
            pthread_join(threads[threadNum].threadHandle, (void**)&returnValue);
        }
        if (singleFile && threadCount > 1) {
            for (int threadNum = 0; threadNum < threadCount; ++threadNum) {
                sem_destroy(&threads[threadNum].writeSemaphore);
            }
        }
        if (exitStatus != 0) exit(exitStatus);
    }

    if (!silent) stdLog("All threads now terminated", threadCount);    
    
    free(threads);
}



int main(int argC, char ** argV) {
    initializeThreading();
    parseArgs(argC, argV);

    // Set the output mode
    // This is done by setting a function pointer
    switch (fileType) {
        case FILE_TYPE_TEXT:              
            writePrime = writePrimeText;         
            break;

        case FILE_TYPE_SYSTEM_BINARY:     
            writePrime = writePrimeSystemBinary;    
            break;

        case FILE_TYPE_COMPRESSED_BINARY: 
            writePrime = writePrimeCompressedBinary;
            break;
    }


    if (singleFile) {
        theSingleFile = openFileForPrime(startValue, endValue);
    }

    // Initialise the primes array
    if (initFileName) exitError(1,0,"Init from file was removed in V1.2");
    initializeSelf();
    populateLowPrimeMap();

    // Set the debug mask, this is used for verbose priting
    if (verbose) {
        size_t tmp = primeCount;
        APPLY_DEBUG_MASK = 0xF;
        while (tmp & 0xFFFFF700) {
            tmp >>= 8;
            APPLY_DEBUG_MASK = (APPLY_DEBUG_MASK << 8) | 0xFF;
        }
    }

    // Process everything
    runThreads();

    // Free the primes array
    finalSelf();

    // Close the file (this can take some time if it has been cached by the os)
    if (singleFile) {
        // This will close stdout if useStdOut was selected.
        closeFileForPrime(theSingleFile);
    }
    
    return 0;
}

