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

// For memory mapping a file
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#include "prime_64.h"

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

unsigned char removeMask[] = {0xFF, 0xFE, 0xFF, 0xFD, 0xFF, 0xFB, 0xFF, 0xF7, 0xFF, 0xEF, 0xFF, 0xDF, 0xFF, 0xBF, 0xFF, 0x7F};
unsigned char checkMask[] =  {0x00, 0x01, 0x00, 0x02, 0x00, 0x04, 0x00, 0x08, 0x00, 0x10, 0x00, 0x20, 0x00, 0x40, 0x00, 0x80};



void applyPrime(Prime prime, Prime offset, unsigned char * map, size_t mapSize) {
	// An optomisatin can be made here by splitting this function into two.
	// One which starts at prime^2 and one which starts at the beginning
	// process() would be able to determine when to use the two through 
	// binary search.
	
	Prime value = (prime * prime) - offset;
	if (value < 0) {
		value = prime - ((offset - 1) % prime) - 1;
		if (!(value & 1)) value += prime;
	}
	Prime stepSize = prime << 1;
	Prime applyTo = ((Prime) mapSize) << 4;
	while (value < applyTo) {
		#ifdef VERBOSE_DEBUG
		if (verbose) fprintf(stderr,"%s [%s] prime = %lld, value = %lld, map[%lld] = %.2X, "
			"removeMask[%d] = %.2X, result = %.2X, offset = %lld\n", 
			timeNow(), threadString, prime, value, value >> 4, (int) map[value >> 4], (int) value & 0x0F, 
			(int) removeMask[value & 0x0F], (int)(map[value >> 4] & removeMask[value & 0x0F]),
			offset);
		#endif
		map[value >> 4] &= removeMask[value & 0x0F];
		value += stepSize;
	}
}



void initializeSelf() {
	if (!silent) {
		PrimeString startValueString;
		prime_to_str(startValueString, startValue);
		PrimeString endValueString;
		prime_to_str(endValueString, endValue);
		fprintf(stderr, "%s [%s] Running Self initialisation for %s (inc) to %s (ex)\n", 
			timeNow(), threadString, startValueString, endValueString);
	}

	size_t primesAllocated = ALLOC_UNIT;
	primes = mallocSafe(primesAllocated * sizeof(Prime));
	Prime maxRequired;
	prime_sqrt(maxRequired, endValue);
	size_t range = (maxRequired + 15) / 16;
	unsigned char * bitmap = mallocSafe(range * sizeof(unsigned char));
	memset(bitmap, 0xFF, range);

	Prime value = 3ll;
	primeCount = 0;

	while (value <= maxRequired) {

		if (bitmap[value >> 4] & checkMask[value & 0x0F]) { // if value prime
			applyPrime(value, 0, bitmap, range);
			if (primeCount == primesAllocated) {
				if (verbose) {
					PrimeString valueString;
					prime_to_str(valueString, value);
					fprintf(stderr,"%s [%s] Initialized %zd primes, reached value %s (ex)\n", 
						timeNow(), threadString, primeCount, valueString);
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
			if (verbose) fprintf(stderr,"%s [%s] Ruled out %s due to bitmap[%d] = %d and checkMask[%d] = %d\n", 
				timeNow(), threadString, valueString, (int) value >> 4, (int)bitmap[value >> 4], 
				(int)value & 0x0F, (int)checkMask[value & 0x0F]);
		}
		#endif
		value+=2;
	}

	free(bitmap);

	if (!silent) fprintf(stderr,"%s [%s] Prime array now full with %zd primes\n", 
		timeNow(), threadString, primeCount);
}



void finalSelf() {
	free(primes);
}



void initializeFromFile() {
	if (fileCount > 1) {
			fprintf(stderr, "%s [%s] Error: Initialization from multiple files has not yet been implemented\n", 
				timeNow(), threadString);
			exit(1);
	}

	fileHandles = mallocSafe(sizeof(struct FileHandle) * 1); // there can be only one.

		if (!silent) fprintf(stderr, "%s [%s] Initializing from file (%s)\n",
			timeNow(), threadString, files[0]);

	fileHandles[0].fileName = files[0];
	fileHandles[0].handle = open(files[0], O_RDONLY);
	if (fileHandles[0].handle < 0) {
		int lerrno = errno;
		fprintf(stderr, "%s [%s] Error: failed to open file %s\n", 
			timeNow(), threadString, files[0]);
		exitError(1, lerrno);
	}

	if ( fstat(fileHandles[0].handle, &(fileHandles[0].statBuffer)) < 0 ) {
		int lerrno = errno;
			fprintf(stderr, "%s [%s] Error: failed to stat file %s\n", 
				timeNow(), threadString, files[0]);
			exitError(1, lerrno);
	}

	if ( fileHandles[0].statBuffer.st_size % sizeof(Prime) ) {
		int lerrno = errno;
		fprintf(stderr, "%s [%s] Error: unexpected file length %s (%lld) this should be divisible by %zd.\n", 
			timeNow(), threadString, files[0], (Prime)(fileHandles[0].statBuffer.st_size),sizeof(Prime));
			exitError(1, lerrno);
	}

	primes = mmap(0, fileHandles[0].statBuffer.st_size, PROT_READ, MAP_SHARED, fileHandles[0].handle, 0);
	if (primes == MAP_FAILED) {
		int lerrno = errno;
			fprintf(stderr, "%s [%s] Error: failed to memory map file. Is this a regular file %s\n", 
				timeNow(), threadString, files[0]);
			exitError(1, lerrno);
	}


	primeCount = fileHandles[0].statBuffer.st_size / sizeof(Prime);

	if (verbose) fprintf(stderr, "%s [%s] File memory mapped (%s) ... checking file\n",
		timeNow(), threadString, files[0]);

	if (primes[0] != 3ll) {
		fprintf(stderr,"%s [%s] Error: primes[0] is not 3 in %s. Is this a prime initialization file?\n",
			timeNow(), threadString, files[0]);
	}

	for (size_t i = 1; i < primeCount; ++i) {
		if (primes[i] <= primes[i-1]) {
			fprintf(stderr, "%s [%s] Error: primes[%zd] (%lld) is out of sequence with primes[%zd] (%lld) in %s. Is this a prime initialization file?\n", 
				timeNow(), threadString, i-1, primes[i-1], i, primes[i], files[0]);
			exit(1);
		}
		if (!(primes[i] & 1ll)) {
			fprintf(stderr, "%s [%s] Error: primes[%zd] (%lld) is even. Is this a prime initialization file?\n", 
				timeNow(), threadString, i, primes[i], files[0]);
			exit(1);
		}
	}

	if (primes[primeCount-1] * primes[primeCount-1] < endValue) {
		fprintf(stderr, "%s [%s] Error: Prime initialisation file does not contain large enough primes.\n %s is only large enough for primes up to %lld\n", 
			timeNow(), threadString,files[0],primes[primeCount-1] * primes[primeCount-1] );
		exit(1);
	}

	if (verbose) fprintf(stderr, "%s [%s] File checks passed (%s)\n",
		timeNow(), threadString, files[0]);
}




void finalFile() {
	if ( munmap( primes, fileHandles[0].statBuffer.st_size) ) {
		int lerrno = errno;
		fprintf(stderr, "%s [%s] Error: failed to unmap file %s\n", 
			timeNow(), threadString, files[0]);
		exitError(1, lerrno);
	}

	if ( close(fileHandles[0].handle) ) {
		int lerrno = errno;
		fprintf(stderr, "%s [%s] Error: failed to close file %s\n", 
			timeNow(), threadString, files[0]);
		exitError(1, lerrno);
	}
}



void process(Prime from, Prime to, FILE * file) {
	if (!silent) {
		PrimeString fromString;
		prime_to_str(fromString, from);
		PrimeString toString;
		prime_to_str(toString, to);
		fprintf(stderr, "%s [%s] Running process for %s (inc) to %s (ex)\n", 
			timeNow(), threadString, fromString, toString);
	}
	// This program is blind to 2s. So if 2 is in range then print it manually; 
	// it will be the first prime any way.
	if (from <= 2 && to > 2) printValue(file,2ll); 

	if (from < 2) {
		// Process ignores even primes
		// Process doesnt know 1 isnt a prime, so skip it!
		from = 2;
	}
	else {
		// Process expects to be aligned to a even number.
		// process ignores even numbers so adding an extra even
		// number won't cause process to find an extra prime
		if (from & 0x1) --from;
	}
	
	size_t range = (to - from + 15) / 16;
	if (verbose) fprintf(stderr, "%s [%s] Bitmap will contain %zd bytes\n", 
		timeNow(), threadString, range);

	unsigned char * bitmap = mallocSafe(range);
	memset(bitmap, 0xFF, range);

	if (verbose) fprintf(stderr, "%s [%s] Applying all primes\n", 
		timeNow(), threadString, from, to);

	for (size_t i = 0; i < primeCount; ++i) {
		if (verbose) {
			#ifndef VERBOSE_DEBUG
			if (!(i & APPLY_DEBUG_MASK)) {
			#endif
				PrimeString primeValueString;
				prime_to_str(primeValueString, primes[i]);
				fprintf(stderr, "%s [%s] Applying primes %02.2f%% (%zd of %zd [%s])\n", 
					timeNow(), threadString, 100 * ((double)i) / ((double)primeCount),i+1, 
					primeCount, primeValueString);
			#ifndef VERBOSE_DEBUG

			}
			#endif
		}
	applyPrime(primes[i], from, bitmap, range);
	}

	size_t endRange = range -1;
	for (size_t i = 0; i < endRange; ++i) {
		if (!(i & SCAN_DEBUG_MASK)) {
			if (verbose) fprintf(stderr, "%s [%s] Scanning for new primes %02.2f%%\n", 
				timeNow(), threadString, 100 * ((double) i)/((double) range));
		}
		if (bitmap[i]) {
			for (int j = 1; j < 16; j+=2) {
				if (bitmap[i] & checkMask[j]) {
					Prime value = from + (((Prime) i) << 4) + j;
					#ifdef VERBOSE_DEBUG
					if (verbose) fprintf(stderr,"%s [%s] found prime = %lld, map[%lld] = %.2X, checkMask[%d] = %.2X, "
						"result = %.2X, from = %lld\n", 
						timeNow(), threadString, value, (Prime)i, (int) bitmap[i], (int) j, 
						(int) checkMask[j], (int)(bitmap[i] & checkMask[j]), from);
					#endif
					printValue(file,value);
				}
			}
		}
	}
	if (verbose) fprintf(stderr, "%s [%s] Scanning last byte of bitmap for new primes\n", 
		timeNow(), threadString, startValue, endValue);
	if (bitmap[endRange]) {
		for (int j = 1; j < 16; j+=2) {
			if (bitmap[endRange] & checkMask[j]) {
				Prime value = from + (((Prime) endRange) << 4) + j;
				#ifdef VERBOSE_DEBUG
				if (verbose) fprintf(stderr,"%s [%s] found prime = %lld, map[%lld] = %.2X, checkMask[%d] = %.2X, "
					"result = %.2X, from = %lld\n", 
					timeNow(), threadString, value, (Prime)endRange, (int) bitmap[endRange], (int) j, 
					(int) checkMask[j], (int)(bitmap[endRange] & checkMask[j]), from);
				#endif
				if (value < endValue) printValue(file,value);
			}
		}
	}

	if (verbose) fprintf(stderr, "%s [%s] All primes have now been discovered between %lld (inc) and %lld (ex)\n",
		timeNow(), threadString, from, to);

	free(bitmap);
}



void processToFile(Prime from, Prime to) {
	if (useStdout) {
		process(from, to, stdout);
	}
	else {
		char fileName[FILENAME_MAX];
		PrimeString fromString;
		prime_to_str(fromString, from);
		PrimeString toString;
		prime_to_str(toString, to);
		snprintf(fileName, FILENAME_MAX,"%s/%s%19s%s%19s%s",
			fileDir,filePrefix,fromString,fileInfix,toString,fileSuffix);
			
		for (int i=0; fileName[i]; ++i) if (fileName[i] == ' ') fileName[i] = '0';

		if (!silent) fprintf(stderr, "%s [%s] Starting new prime file: %s\n",
			timeNow(), threadString, fileName);
		FILE * file = fopen(fileName, "wx");
		if (!file) exitError(2, errno);
		process(from, to, file);
		if (fclose(file)) {
			int lerrno = errno;
			fprintf(stderr, "%s [%s] Error: closing prime file reported error "
				"contents may have been truncated. \n%s Error: filename %s\n",
				timeNow(), threadString, timeNow(), fileName);
			exitError(2, lerrno);
		}
		
	}
}



void writeInitializationFile() {
	char fileName[FILENAME_MAX];
	snprintf(fileName, FILENAME_MAX,"%s/%sG%04lld%s",
		fileDir,filePrefix,endValue/1000000000,fileSuffix);
	FILE * file = fopen(fileName, "wx");	
	if (!file) exitError(2, errno);
	
	
	fprintf(stderr, "%s [%s] Writing initialisation to file %s\n",
			timeNow(), threadString, fileName);
	fwrite(primes, sizeof(Prime), primeCount, file);
	

	if (fclose(file)) {
		int lerrno = errno;
		fprintf(stderr, "%s [%s] Error: closing prime file reported error "
			"contents may have been truncated. \n%s Error: filename %s\n",
			timeNow(), threadString, timeNow(), fileName);
		exitError(2, lerrno);
	 }

}



void processAll() {
	Prime from = startValue;
	Prime to = startValue - (startValue % chunkSize) + chunkSize;
	Prime chunkNum = 0;
	while (to < endValue) {
		if (chunkNum % threadCount == (threadNum - 1)) processToFile(from, to);
		from = to;
		to += chunkSize;
		chunkNum++;
	}
	if (from < endValue) {
		if (chunkNum % threadCount == (threadNum - 1)) processToFile(from, endValue);
	}
}



void processAllMultiThread() {
	if (!silent) fprintf(stderr, "%s [%s] Running multithread with %d threads\n", 
		timeNow(), threadString, threadCount);
	for (threadNum = 1; threadNum <= threadCount; ++threadNum) {
		pid_t child = fork();
		if (child ==  -1 ) {  // this will need fixing so that it kills the children
			int lerrno = errno;
			fprintf(stderr, "%s [%s] Error: could not fork for thread %d\n", 
				timeNow(), threadString, threadNum);
			exitError(2, lerrno);
		}
		if (child == 0) {
			sprintf(threadString, "%02d", threadNum);
			if (!silent) fprintf(stderr, "%s [%s] Thread %d started\n", 
				timeNow(), threadString, threadNum);
			processAll();
			if (!silent) fprintf(stderr, "%s [%s] Thread %d finished\n", 
				timeNow(), threadString, threadNum);
			exit(0); // Once the thread has processed all cleanup should be left to the master if there is any
		}
	}

	int exitStatus = 0;

	if (!silent) fprintf(stderr, "%s [%s] All threads now started\n", 
		timeNow(), threadString, threadCount);

	for (threadNum = 0; threadNum < threadCount; ++threadNum) {
		int status;
		pid_t child = wait(&status);
		if (child == -1) {
			int lerrno = lerrno;
			fprintf(stderr, "%s [%s] Error: wait for child thread. There are believed to be %d thread(s) still running\n", 
				timeNow(), threadString, threadCount - threadNum);
			exitError(2, lerrno);
		}
		if (verbose) fprintf(stderr, "%s [%s] Thread completed with return code %d\n", 
			timeNow(), threadString, (int)WEXITSTATUS(status));
		if (WEXITSTATUS(status) != 0 && exitStatus == 0) exitStatus = WEXITSTATUS(status);
	}

	if (!silent) fprintf(stderr, "%s [%s] All threads now terminated\n", 
		timeNow(), threadString, threadCount);    

	if (exitStatus != 0) exit(exitStatus);
}



int main(int argC, char ** argV) {
	parseArgs(argC, argV);
	str_to_prime(startValue, startValueString);
	str_to_prime(endValue, endValueString);
	str_to_prime(chunkSize, chunkSizeString);

	if (fileCount == 0 || initializeOnly) initializeSelf();
	else initializeFromFile();
	
	if (initializeOnly) writeInitializationFile();
	else if (singleThread) processAll();
	else processAllMultiThread();
	
	if (fileCount == 0 || initializeOnly) finalSelf();
	else finalFile();
	return 0;
}

