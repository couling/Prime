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
#include <time.h>
#include <getopt.h>
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
char * * files;
int fileCount;

struct FileHandle {
	char * fileName;
	int handle;
	struct stat statBuffer;
} * fileHandles;

Prime startValue;
Prime endValue;
Prime chunkSize;

int threadCount = 1;
int threadNum = 0;
int singleThread = 0;

int verbose = 0;
int silent = 0;

char * fileDir = ".";
char * filePrefix = "prime.";
char * fileSuffix = ".txt";
char * fileInfix = "-";
int useStdout = 1;
int initializeOnly = 0;

size_t primeCount;                 // Number of primes stored
Prime * primes;                // The array of primes

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



void setDefaults() {
	prime_set_num(startValue,0);
	prime_set_num(endValue, 1000000000);
	prime_set_num(chunkSize, 1000000000);

	threadCount = 1;
	threadNum = 1;
	singleThread = 0;

	verbose = 0;
	silent = 0;

	fileDir = ".";
	filePrefix = "prime.";
	fileSuffix = ".txt";
	fileInfix = "-";
	useStdout = 1;
	initializeOnly = 0;
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
				{"start-trillion",  required_argument, 0, 't'},
				{"end-trillion",  required_argument, 0, 'T'},
				{"chunk-million", required_argument, 0, 'c'},
				{"chunk-billion", required_argument, 0, 'C'},
				{"prefix", required_argument, 0, 1000},
				{"suffix", required_argument, 0, 1001},
				{"infix", required_argument, 0, 1002},
				{"dir", required_argument, 0, 1003},
				{"thread-count", required_argument, 0, 'x'},
				{"thread-num", required_argument, 0, 'X'},
				{"silent", no_argument, 0, 's'},
				{"verbose", no_argument, 0, 'v'},
				{"file", no_argument, 0, 'f'},
				{"initialize-only", no_argument, 0, 'i'},
				{0, 0, 0, 0}
			};    
	static char * shortOptions ="O:o:K:k:M:m:G:g:T:t:C:c:p:X:x:svfi";

	int givenOption;
	// do not allow getopt_long to print an error to stdout if an invalid option is found
	opterr = 0;
	while ((givenOption = getopt_long (argC, argV, shortOptions, longOptions, NULL)) != -1) {
		char * scale;
		int number = 0;
		int threadingNumber = 0;
		switch (givenOption) {
			case 'o': scale = "1"; number = 's'; break;
			case 'O': scale = "1"; number = 'e'; break;
			case 'k': scale = "1000"; number = 's'; break;
			case 'K': scale = "1000"; number = 'e'; break;
			case 'm': scale = "1000000"; number = 's'; break;
			case 'M': scale = "1000000"; number = 'e'; break;
			case 'g': scale = "1000000000"; number = 's'; break;
			case 'G': scale = "1000000000"; number =  'e'; break;
			case 't': scale = "1000000000000"; number = 's'; break;
			case 'T': scale = "1000000000000"; number =  'e'; break;
			case 'c': scale = "1000000"; number = 'c'; break;
			case 'C': scale = "1000000000"; number = 'c'; break;
			case 1000: filePrefix = optarg; break;
			case 1001: fileSuffix = optarg; break;
			case 1002: fileInfix = optarg; break;
			case 1003: fileDir = optarg; break;
			case 'x': threadingNumber = 'x'; break;
			case 'X': threadingNumber = 'X'; break;
			case 's': silent = 1; verbose = 0; break;
			case 'v': verbose = !silent; break;
			case 'f': useStdout = 0; break;
			case 'i': initializeOnly = 1; break;
			case '?': 
				if (optopt) {
					fprintf(stderr, "%s [%s] Error: option -%c\n", 
						timeNow(), threadString, optopt);
				}
				else {
					fprintf(stderr, "%s [%s] Error: option %s\n", 
						timeNow(), threadString, argV[optind-1]);
				}
				exit(1);
				break;
		}

		if (number) {
			Prime value;
			Prime pscale;
			str_to_prime(value, optarg);
			str_to_prime(pscale, scale);
			
			if (number == 's') prime_mul(startValue, value, pscale);
			else if (number == 'e') prime_mul(endValue, value, pscale);
			else if (number == 'c') prime_mul(chunkSize, value, pscale);
		}
		else if (threadingNumber) {
		    long long value;
			char * endptr;
			value = strtoll(optarg, &endptr, 10);
			if (*endptr || value <= 0 || value > 1000) {
				fprintf(stderr, "%s [%s] Error: threading number %s\n", 
					timeNow(), threadString, optarg);
				exit(1);
			}
			if (threadingNumber == 'x') {
				threadCount = value;
			}
			else if (threadingNumber == 'X') {
				threadNum = value;
			}
		}
	}

	// We default to multithreading with 1 thread...
	// This is more normally known as single threading so
	// lets not waste time with the multithreading functions.
	if (threadCount == 1) singleThread = 1;

	int errorFlag = 0;

	if (endValue <= startValue) {
		PrimeString startValueString;
		prime_to_str(startValueString, startValue);
		PrimeString endValueString;
		prime_to_str(endValueString, endValue);
		
		fprintf(stderr, "%s [%s] Error: end value (%s) is not larger than start value (%s).\n", 
			timeNow(), threadString, endValueString, startValueString);
		errorFlag = 1;
	}

	if (strchr(filePrefix, '/') || strchr(fileInfix, '/') || strchr(fileSuffix, '/')) {
		fprintf(stderr, "%s [%s] Error: '/' found in file name, directories MUST be specified using --dir\n", 
			timeNow(), threadString, endValue, startValue);
		errorFlag = 1;
	}

	if (threadCount < 1) {
		fprintf(stderr, "%s [%s] Error: invalid thread-count (%d). Must be 1 or more.\n",
			timeNow(), threadString, threadCount);
		errorFlag = 1;
	}

	if (threadNum < 0 || threadNum > threadCount) {
		fprintf(stderr, "%s [%s] Error: invalud thread-num (%d).  Must be between 1 and thread-count (%d).\n",
			timeNow(), threadString, threadNum, threadCount);
		errorFlag = 1;
	}

	if (threadCount > 1 && useStdout) {
		fprintf(stderr, "%s [%s] Warning: Using thread-count > 1 (multithreading) on the stdout will not produce contiguous primes.\n"
		"%s [%s] Warning: Recommend using --file or manually dividing threads by range (-g and -G).\n", timeNow(), threadString, timeNow(), threadString);
	}

	if (errorFlag) exit(1);

	fileCount = optind - argC;
	files = argV + optind;
}



int main(int argC, char ** argV) {
	setDefaults();
	parseArgs(argC, argV);

	if (fileCount == 0 || initializeOnly) initializeSelf();
	else initializeFromFile();
	
	if (initializeOnly) writeInitializationFile();
	else if (singleThread) processAll();
	else processAllMultiThread();
	
	if (fileCount == 0 || initializeOnly) finalSelf();
	else finalFile();
	return 0;
}

