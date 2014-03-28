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

//#define VERBOSE_DEBUG

#define ALLOC_UNIT 0x100000
#define APPLY_DEBUG_MASK 0xFFFF
#define SCAN_DEBUG_MASK 0x3FFFFF


// For threading
typedef struct ThreadDescriptor {
	int threadNum;
	pthread_t threadHandle;
	sem_t writeSemaphore;
	sem_t * nextThreadWriteSemaphore;
} ThreadDescriptor;


ThreadDescriptor * threads;


// For file based initialisation
int initFileHandle;
off_t initFileSize;


//  Functions for writing primes
typedef void (* WritePrimeFunction)(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, FILE * file);
void writePrimeText(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, FILE * file);
void writePrimeSystemBinary(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, FILE * file );
void writePrimeCompressedBinary(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, FILE * file );

WritePrimeFunction writePrime = writePrimeText;

// The single file to write to (if single file is enabled);
FILE * theSingleFile;


// All primes less than or sqrt(endValue)
// Generating these is what initialisation is for
size_t primeCount;                 // Number of primes stored
size_t primesAllocated;            // Number of primes the array can store
Prime * primes;                    // The array of primes


// Maps used to operate on compressed prime bitmasks
unsigned char removeMask[] = {0xFF, 0xFE, 0xFF, 0xFD, 0xFF, 0xFB, 0xFF, 0xF7, 0xFF, 0xEF, 0xFF, 0xDF, 0xFF, 0xBF, 0xFF, 0x7F};
unsigned char checkMask[] =  {0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};

static void applyPrime(Prime prime, Prime offset, unsigned char * map, size_t mapSize) {
	// An optomisatin can be made here by splitting this function into two.
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
	prime_mul_num(applyTo, applyTo, 16);
	while (prime_lt(value,applyTo)) {
		Prime tmp;
		prime_div_num(tmp, value, 16);
		map[prime_get_num(tmp)] &= removeMask[prime_get_num(value) & 0x0F];
		prime_add_prime(value, value, stepSize);
	}
}



static void getPrimeFromMap(Prime value, Prime offset, size_t byteIndex, int bitIndex) {
    prime_set_num(value, byteIndex);
    prime_mul_num(value, value, 16);
	prime_add_num(value, value, byteIndex);
	prime_add_prime(value, value, offset);
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
	prime_sqrt(maxRequired, maxRequired);
	Prime pRange;
	prime_add_num(pRange, maxRequired, 15);
	prime_div_num(pRange, pRange, 16);
	size_t range = prime_get_num(pRange);
	unsigned char * bitmap = mallocSafe(range * sizeof(unsigned char));
	memset(bitmap, 0xFF, range);
	primeCount = 0;


	// Evaluate all primes up to sqrt(sqrt(endValue))
	// Each prime found here needs to be applied to the map
	Prime zero;
	prime_set_num(zero, 0);
    prime_sqrt(maxRequired, maxRequired);
    prime_add_num(pRange, maxRequired, 15);
    prime_div_num(pRange, pRange, 16);
    size_t endRange = prime_get_num(pRange);
	size_t i = 0;
    while (i <= endRange) {
        if (verbose && !(i & SCAN_DEBUG_MASK)) 
            stdLog("Scanning for new primes %02.2f%%", 100 * ((double) i)/((double) range));
        
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
            stdLog("Scanning for new primes %02.2f%%", 100 * ((double) i)/((double) range));
        
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



void finalSelf() {
	free(primes);
}



void initializeFromFile() {
	if (!silent) stdLog("Initializing from file (%s)", initFileName);

	// Open the file
	initFileHandle = open(initFileName, O_RDONLY);
	if (initFileHandle < 0) exitError(1, errno, "failed to open file %s", initFileName);
	
	// Get the file size and check that it makes sense
	struct stat statBuffer;
	if ( fstat(initFileHandle, &statBuffer) < 0 ) exitError(1, errno, "failed to stat file %s", initFileName);
	
	initFileSize = statBuffer.st_size;
	if ( initFileSize % sizeof(Prime) ) 
		exitError(1, 0, "unexpected file length %s (%lld) this should be divisible by %zd", 
			initFileName, (long long)initFileSize, sizeof(Prime));

	primeCount = primesAllocated = initFileSize / sizeof(Prime);
	if ( primeCount < 2 )
		exitError(1, 0, "unexpected file length %s (%lld) should be at lease two primes long %zd bytes",
			initFileName, (long long) initFileSize, sizeof(Prime) * 2);

	// Memory map the full file
	primes = mmap(0, initFileSize, PROT_READ, MAP_SHARED, initFileHandle, 0);
	if (primes == MAP_FAILED) exitError(1, errno, "failed to memory map init file %s", initFileName);

	
	// Perform sanity checks on file.
	if (verbose) stdLog("File memory mapped (%s) ... checking file", initFileName);
	
	// The first prime is always 3 the second always 5
	// This is used as a sort of file signature and helps ensure we're using the correct format.
	Prime comp;
	prime_set_num(comp, 3);
	if (!prime_eq(primes[0], comp)) {
		PrimeString s;
		prime_to_str(s, primes[0]);
		exitError(1, 0, "primes[0] is not 3 but %s in %s", s, initFileName);
	}
	prime_set_num(comp, 5);
    if (!prime_eq(primes[1], comp)) {
        PrimeString s;
        prime_to_str(s, primes[1]);
        exitError(1, 0, "primes[1] is not 5 but %s in %s", s, initFileName);
    }

	// Primes are listed in ascending numerical order and are all odd.
	for (size_t i = 1; i < primeCount; ++i) {
		if (prime_lt(primes[i], primes[i-1])) {
			PrimeString p1, p2;
			prime_to_str(p1, primes[i]);
			prime_to_str(p2, primes[i-1]);
			exitError(1, 0, "primes[%zd] (%s) is out of sequence with primes[%zd] (%s) in %s", 
				i, p1, i-1, p2, initFileName);
		}
		if (!(prime_is_odd(primes[i]))) {
			PrimeString s;
			prime_to_str(s, primes[i]);
			exitError(1, 0, "primes[%zd] (%s) is even in %s", i, s, initFileName);
		}
	}

	// The file reaches a high enough prime for the task in hand ie: sqrt(endValue)
	Prime maxValue;
	prime_mul_prime(maxValue, primes[primeCount-1], primes[primeCount-1]);
	if (prime_lt(maxValue, endValue)) {
		PrimeString s;
		prime_to_str(s, maxValue);
		exitError(1, 0, "Prime initialisation too small. %s is only large enough for primes up to %s", 
			initFileName, s );
	}
	

	if (verbose) stdLog("File checks passed (%s)", initFileName);
}




void finalFile() {
	// Unmap the file and close the handle
	if ( munmap( primes, initFileSize) ) exitError(1, errno, "failed to unmap file %s", initFileName);
	if ( close(initFileHandle) )  exitError(1, errno, "failed to close file %s", initFileName);
}




void writePrimeText(Prime from, Prime to, size_t range, unsigned char * bitmap, FILE * file) {
    int threadNum;
    if (singleFile && threadCount > 1) {
        threadNum = (*((int*)pthread_getspecific(threadNumKey))) -1;
        sem_wait(&threads[threadNum].writeSemaphore);
    }
	Prime prime_2;
	prime_set_num(prime_2, 2);
	if (prime_le(from, prime_2) && prime_ge(to, prime_2)) {
		fprintf(file, "2\n");
	}
    size_t endRange = range -1;
    for (size_t i = 0; i < endRange; ++i) {
        if (verbose && !(i & SCAN_DEBUG_MASK)) 
            stdLog("Scanning for new primes %02.2f%%", 100 * ((double) i)/((double) range));

        if (bitmap[i]) {
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[i] & checkMask[j]) {
                    Prime value;
					getPrimeFromMap(value, from, i, j);
					PrimeString s;
					prime_to_str(s, value);
					fprintf(file, "%s\n", s);
                }
            }
        }
    }

    if (verbose) stdLog("Scanning last byte of bitmap for new primes");
    if (bitmap[endRange]) {
        for (int j = 1; j < 16; j+=2) {
            if (bitmap[endRange] & checkMask[j]) {
                Prime value;
				getPrimeFromMap(value, from, endRange, j);
                if (prime_lt(value, endValue)) {
                    PrimeString s;
                    prime_to_str(s, value);
                    fprintf(file, "%s\n", s);
				}
            }
        }
    }
    if (singleFile && threadCount > 1) {
        sem_post(threads[threadNum].nextThreadWriteSemaphore);
    }
}



void writePrimeSystemBinary(Prime from, Prime to, size_t range, unsigned char * bitmap, FILE * file) {
	int threadNum;
	if (singleFile && threadCount > 1) {
		threadNum = (*((int*)pthread_getspecific(threadNumKey))) -1;
		sem_wait(&threads[threadNum].writeSemaphore);
	}
	Prime prime_2;
    prime_set_num(prime_2, 2);
    if (prime_le(from, prime_2) && prime_ge(to, prime_2)) {
		fwrite(&prime_2, sizeof(Prime), 1, file);
    }
    size_t endRange = range -1;
    for (size_t i = 0; i < endRange; ++i) {
        if (!(i & SCAN_DEBUG_MASK)) {
            if (verbose) stdLog("Scanning for new primes %02.2f%%", 100 * ((double) i)/((double) range));
        }
        if (bitmap[i]) {
            for (int j = 1; j < 16; j+=2) {
                if (bitmap[i] & checkMask[j]) {
                    Prime value = from + (((Prime) i) << 4) + j;
                    #ifdef VERBOSE_DEBUG
                    if (verbose) stdLog("found prime = %lld, map[%lld] = %.2X, checkMask[%d] = %.2X, result = %.2X, from = %lld",
                        value, (Prime)i, (int) bitmap[i], (int) j, (int) checkMask[j], (int)(bitmap[i] & checkMask[j]), from);
                    #endif
                    fwrite(&value, sizeof(Prime), 1, file);
                }
            }
        }
    }
    if (verbose) stdLog("Scanning last byte of bitmap for new primes");
    if (bitmap[endRange]) {
        for (int j = 1; j < 16; j+=2) {
            if (bitmap[endRange] & checkMask[j]) {
                Prime value = from + (((Prime) endRange) << 4) + j;
                #ifdef VERBOSE_DEBUG
                if (verbose) stdLog("found prime = %lld, map[%lld] = %.2X, checkMask[%d] = %.2X, "
                    "result = %.2X, from = %lld", value, (Prime)endRange, (int) bitmap[endRange], (int) j,
                    (int) checkMask[j], (int)(bitmap[endRange] & checkMask[j]), from);
                #endif
                if (value < endValue) fwrite(&value, sizeof(Prime), 1, file);
            }
        }
    }
	if (singleFile && threadCount > 1) {
		sem_post(threads[threadNum].nextThreadWriteSemaphore);
	}
}




void writePrimeCompressedBinary(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, FILE * file ) {
}



void process(Prime from, Prime to, FILE * file) {
	Prime tmp;

	if (!silent) {
		PrimeString fromString;
		prime_to_str(fromString, from);
		PrimeString toString;
		prime_to_str(toString, to);
		stdLog("Running process for %s (inc) to %s (ex)", fromString, toString);
	}
	Prime askFrom;  // This is what were were asked to calculate from
	prime_cp(askFrom, from);
	Prime prime_2;
	prime_set_num(prime_2, 2);
	if (prime_lt(from, prime_2)) {
		// Process ignores even primes
		// Process doesnt know 1 isnt a prime, so skip it!
		prime_set_num(from, 2);
		prime_set_num(askFrom, 2); // yup if we're asked to start from 1 or less
		                            // ignore the stupid user and calculate from 2
	}
	else {
		// Process expects to be aligned to a even number.
		// process ignores even numbers so adding an extra even to the start of the range
		// number won't cause process to find an extra prime
		if (prime_is_odd(from)) prime_sub_num(from, from, 1);
	}
	
	prime_sub_prime(tmp, to, from);
	size_t range = (prime_get_num(tmp) + 15) / 16;
	if (verbose) stdLog("Bitmap will contain %zd bytes", range);

	unsigned char * bitmap = mallocSafe(range);
	memset(bitmap, 0xFF, range);

	if (verbose) stdLog("Applying all primes", from, to);

	for (size_t i = 0; i < primeCount; ++i) {
		if (verbose) {
			if (!(i & APPLY_DEBUG_MASK)) {
				PrimeString primeValueString;
				prime_to_str(primeValueString, primes[i]);
				stdLog("Applying primes %02.2f%% (%zd of %zd [%s])", 
					100 * ((double)i) / ((double)primeCount),i+1, primeCount, primeValueString);
			}
		}
		applyPrime(primes[i], from, bitmap, range);
	}

	writePrime(askFrom, to, range, bitmap, file);

	if (verbose) stdLog("All primes have now been discovered between %lld (inc) and %lld (ex)", from, to);

	free(bitmap);
}



void * processAllChunks(void * threadPt) {
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
				FILE * file = openFileForPrime(from, to);
				process(from, to, file);
				if (fclose(file)) {
            		exitError(2, errno, "closing prime file reported error - contents may have been truncated");
		        }
			}
		}
		prime_cp(from,to);
		prime_add_prime(to, to, chunkSize);
		chunkNum++;
	}
	if (prime_lt(from, endValue)) {
		if (chunkNum % threadCount == (thread->threadNum - 1)) {
			if (useStdout) process(from, endValue, stdout);
			else if (singleFile) process(from, endValue, theSingleFile);
			else {
                FILE* file = openFileForPrime(from, endValue);
                process(from, endValue, file);
                if (fclose(file)) {
                    exitError(2, errno, "closing prime file reported error - contents may have been truncated");
                }

            }
		}
	}
	if (!silent) stdLog("Thread %d finished", thread->threadNum);
	return NULL;
}




void runThreads() {
	if (!silent) stdLog("Running multithread with %d threads", threadCount);

	threads = mallocSafe(sizeof(struct ThreadDescriptor) * threadCount);

	if (threadCount == 1) {
		threads[0].threadNum = 1;
		processAllChunks(&(threads[0]));
		threads[0].threadNum = 0;
	}
	else{
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
		if (!silent) stdLog("All threads now started");
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

	if (initFileName) initializeFromFile();
	else initializeSelf();
	
	if (singleFile) {
		if (useStdout) theSingleFile = stdout;
		else theSingleFile = openFileForPrime(startValue, endValue);
	}

	// This line kicks off the processing
	runThreads();

	if (singleFile) {
		// This will close stdout if useStdOut was selected.
		if (fclose(theSingleFile)) {
	        exitError(2, errno, "closing prime file reported error - contents may have been truncated");
        }
	}
	
	if (initFileName) finalFile(); 
	else finalSelf();

	return 0;
}

