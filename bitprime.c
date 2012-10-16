#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>
#include <string.h>

//#define VERBOSE_DEBUG

#define ALLOC_UNIT 512

// Config parameters
char * * files;
int fileCount;
long long startValue = 0;
long long endValue = 1000000ll;
long long chunkSize = 1000000000ll;

size_t primeCount;                 // Number of primes stored
long long * primes;                // The array of primes

unsigned char removeMask[] = {0xFF, 0xFE, 0xFF, 0xFD, 0xFF, 0xFB, 0xFF, 0xF7, 0xFF, 0xEF, 0xFF, 0xDF, 0xFF, 0xBF, 0xFF, 0x7F};
unsigned char checkMask[] =  {0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};

#define PRINT_VALUE(VALUE) printf("%lld\n", VALUE )

char timeNow[100];

void setTimeNow() {
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    
    strftime (timeNow,100,"%a %b %d %H:%M:%S %Y",timeinfo);
}



void * mallocSafe(size_t bytes) {
    setTimeNow();
    void * result = malloc(bytes);
    if (!result) {
        setTimeNow();
        fprintf(stderr, "%s Error: Could not allocate %zd bytes\n", timeNow, bytes);
        exit(255);
    }
    return result;
}



void * reallocSafe(void * existing, size_t bytes) {
    void * result = realloc(existing, bytes);
    if (!result) {
        setTimeNow();
        fprintf(stderr, "%s Error: Could not reallocate to %zd bytes\n", timeNow, bytes);
        exit(255);
    }
    return result;
}



void applyPrime(long long prime, long long offset, unsigned char * map, size_t mapSize) {
    long long value = prime - ((offset - 1) % prime) - 1;
    if (!(value & 1)) value += prime;
    /*long long value = prime * prime;
    if (value < offset) {
        value = prime - ((offset - 1) % prime) - 1;
        if (!(value & 1)) value += prime;
    }
    else {
        value -= offset;
    }*/
    long long stepSize = prime << 1;
    long long applyTo = ((long long) mapSize) << 4;
    while (value < applyTo) {
        #ifdef VERBOSE_DEBUG
        setTimeNow();
        fprintf(stderr,"%s prime = %lld, value = %lld, map[%lld] = %d, removeMask[%d] = %d, result = %d\n", 
            timeNow, prime, value, value >> 4, (int) map[value >> 4], (int) value & 0x0F, 
            (int) removeMask[value & 0x0F], (int)(map[value >> 4] & removeMask[value & 0x0F]));
        #endif
        map[value >> 4] &= removeMask[value & 0x0F];
        value += stepSize;
    }
}



void initializeSelf() {
    setTimeNow();
    fprintf(stderr, "%s Running Self initialisation for %lld (inc) to %lld (ex)\n", 
        timeNow, startValue, endValue);

    size_t primesAllocated = ALLOC_UNIT;
    primes = mallocSafe(primesAllocated * sizeof(long long));
    size_t range = ALLOC_UNIT;
    unsigned char * bitmap = mallocSafe(range * sizeof(unsigned char));
    memset(bitmap, 0xFF, range);

    long long value=3ll;
    primeCount = 0;

    while (1) {
        long long maxSize = (((long long) range) << 4);
        while (value < maxSize) {
            if (bitmap[value >> 4] & checkMask[value & 0x0F]) {
                applyPrime(value, 0, bitmap, range);
                if (primeCount == primesAllocated) {
                    setTimeNow();
                    fprintf(stderr,"%s Initialized %zd primes, reached value %lld (ex)\n", 
                        timeNow, primeCount, value);
                    primesAllocated += ALLOC_UNIT;
                    primes = reallocSafe(primes, primesAllocated * sizeof(long long));
                }
                primes[primeCount] = value;
                ++primeCount;
                if (value * value > endValue) goto Complete;
            }
            #ifdef VERBOSE_DEBUG
            else {
                setTimeNow();
                fprintf(stderr,"%s Ruled out %lld due to bitmap[%d] = %d and checkMask[%d] = %d\n", 
                    timeNow, value, (int) value >> 4, (int)bitmap[value >> 4], 
                    (int)value & 0x0F, (int)checkMask[value & 0x0F]);
            }
            #endif
            value+=2;
        }
        bitmap = reallocSafe(bitmap, (range + ALLOC_UNIT) * sizeof(unsigned char)); 
        long long offset = ((long long) range) << 4;
        unsigned char * newbitmap = bitmap +  range;
        memset(newbitmap, 0xFF, ALLOC_UNIT * sizeof(unsigned char));
        for (size_t i = 0; i < primeCount; ++i)
            applyPrime(primes[i], offset, newbitmap, ALLOC_UNIT);
        range += ALLOC_UNIT;
    }

    Complete:

    free(bitmap);

    setTimeNow();
    fprintf(stderr,"%s Prime array now full with %d primes\n", timeNow, primeCount);

}



void finalSelf() {
    free(primes);
}



void initializeFromFile() {
    setTimeNow();
    fprintf(stderr, "%s Initialization from file has not yet been implemented\n", timeNow, startValue, endValue);
    exit(1);
}




void finalFile() {
    // Can never get here as files aren't implemented
}



void process(long long from, long long to) {
    if (from <= 2) {
        // Process ignores even primes
        // Process doesnt know 1 isnt a prime, so skip it!
        from = 2;
    }
    else {
        // Process expects to aligned to a even number.
        // If it's odd start a number earlier, the even can never be found
        // so it doesn't matter that we start on it.
        if (from & 0x1) --from;
    }

    setTimeNow();
    fprintf(stderr, "%s Running process for %lld (inc) to %lld (ex)\n", timeNow, from, to);
    if (from <= 2 && to > 2) PRINT_VALUE(2ll);
    
    size_t range = (to - from + 0x0F) >> 4;
    setTimeNow();
    fprintf(stderr, "%s Bitmap will contain %zd bytes\n", timeNow, range);

    unsigned char * bitmap = mallocSafe(range);
    memset(bitmap, 0xFF, range);

    setTimeNow();
    fprintf(stderr, "%s Applying all primes\n", timeNow, from, to);

    for (size_t i = 0; i < primeCount; ++i) {
        #ifndef VERBOSE_DEBUG
        if (!(i & 0x7FF)) {
        #endif
            setTimeNow();
            fprintf(stderr, "%s Applying prime %zd of %zd (%lld)\n", timeNow, i+1, primeCount, primes[i]);
        #ifndef VERBOSE_DEBUG
        }
        #endif
	applyPrime(primes[i], from, bitmap, range);
    }

    size_t endRange = range -1;
    for (size_t i = 0; i < endRange; ++i) {
        if (!(i & 0x3FFFFF)) {
            setTimeNow();
            fprintf(stderr, "%s Scanning for new primes %2.2f%%\n", 
                timeNow, 100*((double) i)/((double) range));
        }
        if (bitmap[i]) {
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[i] & checkMask[j]) {
                    long long value = from + (((long long) i) << 4) + j;
                    #ifdef VERBOSE_DEBUG
                    setTimeNow();
                    fprintf(stderr,"%s found prime = %lld, map[%lld] = %d, checkMask[%d] = %d, "
                        "result = %d, startValue = %lld\n", 
                        timeNow, value, (long long)i, (int) bitmap[i], (int) j, 
                        (int) checkMask[j], (int)(bitmap[i] & checkMask[j]), from);
                    #endif
                    PRINT_VALUE(value);
                }
            }
        }
    }
    setTimeNow();
    fprintf(stderr, "%s Scanning last byte of bitmap for new primes\n", timeNow, startValue, endValue);
    if (bitmap[endRange]) {
        for (int j = 1; j < 16; j+=2) {
            if (bitmap[endRange] & checkMask[j]) {
                long long value = from + (((long long) endRange) << 4) + j;
                #ifdef VERBOSE_DEBUG
                setTimeNow();
                fprintf(stderr,"%s found prime = %lld, map[%lld] = %d, checkMask[%d] = %d, "
                    "result = %d, startValue = %lld\n", 
                    timeNow, value, (long long)endRange, (int) bitmap[endRange], (int) j, 
                    (int) checkMask[j], (int)(bitmap[endRange] & checkMask[j]), from);
                #endif
                if (value < endValue) PRINT_VALUE(value);
            }
        }
    }

    setTimeNow();
    fprintf(stderr, "%s All primes have now been discovered between %lld (inc) and %lld (ex)\n",
        timeNow, from, to);

    free(bitmap);
}



void parseArgs(int argC, char ** argV) {    

    static struct option longOptions[] =
             {
               {"start",  required_argument, 0, 'o'},
               {"end",  required_argument, 0, 'O'},
               {"start-thousand", required_argument, 0, 'k'},
               {"end-thousand", required_argument, 0, 'K'},
               {"start-million",  required_argument, 0, 'm'},
               {"end-million",  required_argument, 0, 'M'},
               {"start-billion",  required_argument, 0, 'g'},
               {"end-billion",  required_argument, 0, 'G'},
               {"chunk-million", required_argument, 0, 'c'},
               {"chunk-billion", required_argument, 0, 'C'},
               {0, 0, 0, 0}
             };    
    static char * shortOptions ="O:o:K:k:M:m:G:g:t:";

    int givenOption;
    
    while ((givenOption = getopt_long (argC, argV, shortOptions, longOptions, NULL)) != -1) {
        long long scale;
        int number = 0;
        switch (givenOption) {
            case 'o': scale = 1ll; number = 's'; break;
            case 'O': scale = 1ll; number = 'e'; break;
            case 'k': scale = 1000ll; number = 's'; break;
            case 'K': scale = 1000ll; number = 'e'; break;
            case 'm': scale = 1000000ll; number = 's'; break;
            case 'M': scale = 1000000ll; number = 'e'; break;
            case 'g': scale = 1000000000ll; number = 's'; break;
            case 'G': scale = 1000000000ll; number =  'e'; break;
            case 'c': scale = 1000000ll; number = 'c'; break;
            case 'C': scale = 1000000000ll; number = 'c'; break;
            default: number = 0; break;
        }

        if (number) {
            long long value;

            char * endptr;
            value = strtoll(optarg, &endptr, 10);
            if (*endptr || value < 0) {
                setTimeNow();
                fprintf(stderr, "%s Error: invalid number %s\n", timeNow, optarg);
                exit(1);
            }

            if (number == 's') startValue = value * scale;
            else if (number == 'e') endValue = value * scale;
            else if (number == 'c') {
                if (value == 0) {
                    setTimeNow();
                    fprintf(stderr, "%s Error: invalid number %s\n", timeNow, optarg);
                    exit(1);
                }
                chunkSize = value * scale;
            }
        }
    }

    fileCount = optind - argC;
    files = argV + optind;
}



int main(int argC, char ** argV) {
    parseArgs(argC, argV);

    if (fileCount == 0) initializeSelf();
    else initializeFromFile();

    long long from = startValue;
    long long to = startValue - (startValue % chunkSize) + chunkSize;
    while (to < endValue) {
        process(from, to);
        from = to;
        to += chunkSize;
    }
    if (from < endValue) {
        process(from, endValue);
    }
    
    if (fileCount == 0) finalSelf();
    else finalFile();
    return 0;
}

