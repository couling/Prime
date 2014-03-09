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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <pthread.h>
#include <semaphore.h>

// For memory mapping a file
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef PRIME_64
# include "prime_64.h"
#endif

#ifdef PRIME_GMP
# include "primegmp.h"
#endif

#include "prime_shared.h"

//#define VERBOSE_DEBUG

#define ALLOC_UNIT 0x100000
#define APPLY_DEBUG_MASK 0xFFFF
#define SCAN_DEBUG_MASK 0x3FFFFF

// Config parameters

Prime startValue;
Prime endValue;
Prime chunkSize;

struct FileHandle {
	char * fileName;
	int handle;
	struct stat statBuffer;
} * fileHandles;

size_t primeCount;                 // Number of primes stored
Prime * primes;                    // The array of primes


typedef struct ThreadDescriptor {
	int threadNum;
	pthread_t threadHandle;
	sem_t writeSemaphore;
	sem_t * nextThreadWriteSemaphore;
} ThreadDescriptor;

ThreadDescriptor * threads;

FILE * theSingleFile;

unsigned char removeMask[] = {0xFF, 0xFE, 0xFF, 0xFD, 0xFF, 0xFB, 0xFF, 0xF7, 0xFF, 0xEF, 0xFF, 0xDF, 0xFF, 0xBF, 0xFF, 0x7F};
unsigned char checkMask[] =  {0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};

void writePrimeText(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, FILE * file);
void writePrimeSystemBinary(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, FILE * file );

typedef void (* WritePrimeFunction)(Prime startValue, Prime endValue, size_t range, unsigned char * bitmap, FILE * file);
WritePrimeFunction writePrime = writePrimeText;

void applyPrime(Prime prime, Prime offset, unsigned char * map, size_t mapSize) {
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
		#ifdef VERBOSE_DEBUG
		if (verbose) stdLog("prime = %lld, value = %lld, map[%lld] = %.2X, "
			"removeMask[%d] = %.2X, result = %.2X, offset = %lld", 
			prime, value, value >> 4, (int) map[value >> 4], (int) value & 0x0F, 
			(int) removeMask[value & 0x0F], (int)(map[value >> 4] & removeMask[value & 0x0F]),
			offset);
		#endif
		Prime tmp;
		prime_div_num(tmp, value, 16);
		map[prime_get_num(tmp)] &= removeMask[prime_get_num(value) & 0x0F];
		prime_add_prime(value, value, stepSize);
	}
}



void initializeSelf() {
	if (!silent) {
		PrimeString startValueString;
		prime_to_str(startValueString, startValue);
		PrimeString endValueString;
		prime_to_str(endValueString, endValue);
		stdLog("Running Self initialisation for %s (inc) to %s (ex)", 
			startValueString, endValueString);
	}

	size_t primesAllocated = ALLOC_UNIT;
	primes = mallocSafe(primesAllocated * sizeof(Prime));
	Prime maxRequired;
	prime_sqrt(maxRequired, endValue);
	Prime pRange;
	prime_add_num(pRange, maxRequired, 15);
	prime_div_num(pRange, pRange, 16);
	size_t range = prime_get_num(prange);
	unsigned char * bitmap = mallocSafe(range * sizeof(unsigned char));
	memset(bitmap, 0xFF, range);

	Prime value;
	prime_set_num(value, 3);
	primeCount = 0;

	while (prime_lt(value,maxRequired)) {

		if (bitmap[value >> 4] & checkMask[value & 0x0F]) { // if value prime
			applyPrime(value, 0, bitmap, range);
			if (primeCount == primesAllocated) {
				if (verbose) {
					PrimeString valueString;
					prime_to_str(valueString, value);
					stdLog("Initialized %zd primes, reached value %s (ex)", primeCount, valueString);
				}
				primesAllocated += ALLOC_UNIT;
				primes = reallocSafe(primes, primesAllocated * sizeof(Prime));
			}
			primes[primeCount] = value;
			++primeCount;
		}
		#ifdef VERBOSE_DEBUG
		else {
			PrimeString valueString;
			prime_to_str(valueString, value);
			if (verbose) stdLog("Ruled out %s due to bitmap[%d] = %d and checkMask[%d] = %d", 
				valueString, (int) value >> 4, (int)bitmap[value >> 4], 
				(int)value & 0x0F, (int)checkMask[value & 0x0F]);
		}
		#endif
		value+=2;
	}

	free(bitmap);

	if (!silent) stdLog("Prime array now full with %zd primes", primeCount);
}



void finalSelf() {
	free(primes);
}



void initializeFromFile() {
	if (fileCount > 1) {
			exitError(1,0, "Initialization from multiple files has not yet been implemented");
	}

	fileHandles = mallocSafe(sizeof(struct FileHandle) * 1); // there can be only one.

		if (!silent) stdLog("Initializing from file (%s)", files[0]);

	fileHandles[0].fileName = files[0];
	fileHandles[0].handle = open(files[0], O_RDONLY);
	if (fileHandles[0].handle < 0) {
		exitError(1, errno, "failed to open file %s", files[0]);
	}

	if ( fstat(fileHandles[0].handle, &(fileHandles[0].statBuffer)) < 0 ) {
		exitError(1, errno, "failed to stat file %s", files[0]);
	}

	if ( fileHandles[0].statBuffer.st_size % sizeof(Prime) ) {
		exitError(1, errno, "unexpected file length %s (%lld) this should be divisible by %zd.", 
			files[0], (Prime)(fileHandles[0].statBuffer.st_size),sizeof(Prime));
	}

	primes = mmap(0, fileHandles[0].statBuffer.st_size, PROT_READ, MAP_SHARED, fileHandles[0].handle, 0);
	if (primes == MAP_FAILED) {
		exitError(1, errno, "failed to memory map file. Is this a regular file %s", files[0]);
	}


	primeCount = fileHandles[0].statBuffer.st_size / sizeof(Prime);

	if (verbose) stdLog("File memory mapped (%s) ... checking file", files[0]);

	if (primes[0] != 3ll) {
		exitError(1, 0, "primes[0] is not 3 in %s. Is this a prime initialization file?", files[0]);
	}

	for (size_t i = 1; i < primeCount; ++i) {
		if (primes[i] <= primes[i-1]) {
			exitError(1, 0, "primes[%zd] (%lld) is out of sequence with primes[%zd] (%lld) in %s. Is this a prime initialization file?", 
				i-1, primes[i-1], i, primes[i], files[0]);
		}
		if (!(primes[i] & 1ll)) {
			exitError(1, 0, "primes[%zd] (%lld) is even. Is this a prime initialization file?", i, primes[i], files[0]);
		}
	}

	if (primes[primeCount-1] * primes[primeCount-1] < endValue) {
		exitError(1, 0, "Prime initialisation file does not contain large enough primes. %s is only large enough for primes up to %lld", 
			files[0],primes[primeCount-1] * primes[primeCount-1] );
	}

	if (verbose) stdLog("File checks passed (%s)", files[0]);
}




void finalFile() {
	if ( munmap( primes, fileHandles[0].statBuffer.st_size) ) {
		exitError(1, errno, "failed to unmap file %s", files[0]);
	}

	if ( close(fileHandles[0].handle) ) {
		exitError(1, errno, "failed to close file %s", files[0]);
	}
}




void writePrimeText(Prime from, Prime to, size_t range, unsigned char * bitmap, FILE * file) {
    int threadNum;
    if (singleFile && !singleThread) {
        threadNum = (*((int*)pthread_getspecific(threadNumKey))) -1;
        sem_wait(&threads[threadNum].writeSemaphore);
    }
	if (from <= 2 && to >=2) {
		printValue(file,2ll);
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
                    printValue(file,value);
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
                if (value < endValue) printValue(file,value);
            }
        }
    }
    if (singleFile && !singleThread) {
        sem_post(threads[threadNum].nextThreadWriteSemaphore);
    }
}



void writePrimeSystemBinary(Prime from, Prime to, size_t range, unsigned char * bitmap, FILE * file) {
	int threadNum;
	if (singleFile && !singleThread) {
		threadNum = (*((int*)pthread_getspecific(threadNumKey))) -1;
		sem_wait(&threads[threadNum].writeSemaphore);
	}
	if (from <= 2 && to >= 2) {
		Prime two = 2;
		fwrite(&two, sizeof(Prime), 1, file); 
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
	if (singleFile && !singleThread) {
		sem_post(threads[threadNum].nextThreadWriteSemaphore);
	}
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
			#ifndef VERBOSE_DEBUG
			if (!(i & APPLY_DEBUG_MASK)) {
			#endif
				PrimeString primeValueString;
				prime_to_str(primeValueString, primes[i]);
				stdLog("Applying primes %02.2f%% (%zd of %zd [%s])", 
					100 * ((double)i) / ((double)primeCount),i+1, primeCount, primeValueString);
			#ifndef VERBOSE_DEBUG
			}
			#endif
		}
		applyPrime(primes[i], from, bitmap, range);
	}

	writePrime(askFrom, to, range, bitmap, file);

	if (verbose) stdLog("All primes have now been discovered between %lld (inc) and %lld (ex)", from, to);

	free(bitmap);
}



FILE * openFileForPrime(Prime from, Prime to) {
	char fileName[FILENAME_MAX];
	PrimeString fromString;
    prime_to_str(fromString, from);
    PrimeString toString;
    prime_to_str(toString, to);
    snprintf(fileName, FILENAME_MAX,"%s/%s%19s%s%19s%s",
        fileDir,filePrefix,fromString,fileInfix,toString,fileSuffix);	
	for (int i=0; fileName[i]; ++i) if (fileName[i] == ' ') fileName[i] = '0';
	if (!silent) stdLog("Starting new prime file: %s", fileName);
	FILE * file = fopen(fileName, "wx");
	if (!file) exitError(2, errno, "Could not create new file: %s", fileName);
	
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
			if (useStdout) process(from, to, stdout);
			else if (singleFile) process(from, to, theSingleFile);
			else {
				FILE* file = openFileForPrime(from, to);
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
		if (singleFile && !singleThread) {
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
		if (singleFile && !singleThread) {
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

	// Set the start and finish values
	Prime scale;
	str_to_prime(scale, startValueScale);
	str_to_prime(startValue, startValueString);
	prime_mul_prime(startValue, startValue, scale);
	str_to_prime(scale, endValueScale);
	str_to_prime(endValue, endValueString);
	prime_mul_prime(endValue, endValue, scale);
	str_to_prime(scale, chunkSizeScale);
	str_to_prime(chunkSize, chunkSizeString);
	prime_mul_prime(chunkSize, chunkSize, scale);

	// Set the output mode
	switch (fileType) {
		case FILE_TYPE_TEXT:          writePrime = writePrimeText;         break; 
		case FILE_TYPE_SYSTEM_BINARY: writePrime = writePrimeSystemBinary; break;
	}

	if (fileCount == 0) initializeSelf();
	else initializeFromFile();
	
	if (!singleFile || useStdout) runThreads();
	else {
		theSingleFile = openFileForPrime(startValue, endValue);
		runThreads();
		if (fclose(theSingleFile)) {
	        exitError(2, errno, "closing prime file reported error - contents may have been truncated");
        }
	}
	
	if (fileCount == 0) finalSelf();
	else finalFile();
	return 0;
}

