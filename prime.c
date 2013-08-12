/* 

Copyright Philip Couling
All rights reserved.

This is an early version of prime finding.  It's pretty slow.
It uses division - dividing each potential new prime by those primes smaller
than the number's square root.

This was obsoleted by bitprime.c

*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>

#define ALLOC_UNIT 128;

int primeCount;
int maxUsable;
int primesAllocated;
long long * primes;
long long startValue = 0;
long long endValue = 1000000ll;

char ** files;
int fileCount;

char timeNow[100];

void setTimeNow() {
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	
	strftime (timeNow,100,"%a %b %d %H:%M:%S %Y",timeinfo);
}



void initializeSelf() {
	setTimeNow();
	fprintf(stderr, "%s Running Self initialisation for %lld (inc) to %lld (ex)\n", 
		timeNow, startValue, endValue);

	primesAllocated = ALLOC_UNIT;
	primes = malloc(primesAllocated * sizeof(long long));
	if (!primes) {
		setTimeNow();
		fprintf(stderr, "%s Could not allocate %d bytes for prime array\n", 
			timeNow, (int) (primesAllocated * sizeof(long long)));
		exit(255);
	}

	// To bootstrap the process we hard code the first three primes
    if (startValue <= 2 && endValue > 2) printf("%lld\n", 2ll);
    if (startValue <= 3 && endValue > 3) printf("%lld\n", 3ll);
    if (startValue <= 3 && endValue > 3) printf("%lld\n", 5ll);
	
	primes[0] = 3ll; 
	primes[1] = 5ll;
	primeCount=2;
	maxUsable=1;
	long long value = 7ll;
    while (1){
        long long nextPrimeRequired = primes[maxUsable] * primes[maxUsable];
        setTimeNow();
        fprintf(stderr, "%s Checking against %d primes - max %lld - checking value %lld\n",
            timeNow, maxUsable, primes[maxUsable-1], value);
        while (value < nextPrimeRequired ) {
            int i=0;
            while (i<maxUsable && value % primes[i]) ++i;
            if (i == maxUsable) {
				if (primeCount == primesAllocated) {
					primesAllocated += ALLOC_UNIT;
					primes = realloc(primes, primesAllocated * sizeof(long long));
					if (!primes) {
						setTimeNow();
						fprintf(stderr, "%s Could not (re)allocate %d bytes for prime array\n", 
							timeNow, (int) (primesAllocated * sizeof(long long)));
						exit(255);
					}
				}
				primes[primeCount] = value;
				++primeCount;
				if (value >= startValue) printf("%lld\n", value);
				if (value * value > endValue) {
					value += 2;
					goto Complete;
				}
            }
            value += 2;
        }
		value += 2;
        ++maxUsable;
    }

	Complete:

	setTimeNow();
	if (startValue < value) {
		fprintf(stderr, "%s All primes have now been discovered between %lld (inc) and %lld (ex)\n",
			timeNow, startValue, value);
		startValue = value;
	}
	else {
		if (!(startValue & 1)) ++startValue;
		while (primes[maxUsable] * primes[maxUsable] < startValue) ++maxUsable;
	}

	fprintf(stderr,"%s Prime array now full with %d primes\n", timeNow, primeCount);

}



void initializeFromFile() {

}



void process() {
	setTimeNow();
	fprintf(stderr, "%s Running process for %lld (inc) to %lld (ex)\n", timeNow, startValue, endValue);
	long long value = startValue;
    while (value < endValue) {
        long long nextPrimeRequired = primes[maxUsable] * primes[maxUsable];
        setTimeNow();
        fprintf(stderr, "%s Checking against %d primes - max %lld - checking value %lld\n",
            timeNow, maxUsable, primes[maxUsable-1], value);
        while (value < nextPrimeRequired) {
			if (value >= endValue) break;
            int i=0;
            while (i<maxUsable && value % primes[i] ) ++i;
            if (i == maxUsable) printf("%lld\n", value);
            value += 2;
        }
		// if this dropped out the loop due to nextPrimeRequred, we know that this value
		// can be factored (next prime required is the square of a prime).
		value += 2;
        ++maxUsable;
    }

	setTimeNow();
    fprintf(stderr, "%s All primes have now been discovered between %lld (inc) and %lld (ex)\n",
        timeNow, startValue, endValue);

}



void final() {
	free(primes);
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
               {"end-billion-",  required_argument, 0, 'G'},
               {"start-trillion",  required_argument, 0, 't'},
			   {"end-trillion",  required_argument, 0, 'T'},
			   {"start-quadrillion",  required_argument, 0, 'p'},
			   {"end-quadrillion",  required_argument, 0, 'P'},
               {0, 0, 0, 0}
             };	
	static char * shortOptions ="O:o:K:k:M:m:G:g:T:t:P:p:";

	int givenOption;
	
	while ((givenOption = getopt_long (argC, argV, shortOptions, longOptions, NULL)) != -1) {
		long long scale;
		int number;
		switch (givenOption) {
			case 'o': scale = 1ll; number = 's'; break;
			case 'O': scale = 1ll; number = 'e'; break;
			case 'k': scale = 1000ll; number = 's'; break;
			case 'K': scale = 1000ll; number = 'e'; break;
			case 'm': scale = 1000000ll; number = 's'; break;
			case 'M': scale = 1000000ll; number = 'e'; break;
			case 'g': scale = 1000000000ll; number = 's'; break;
			case 'G': scale = 1000000000ll; number =  'e'; break;
			case 't': scale = 1000000000000ll; number = 's'; break;
			case 'T': scale = 1000000000000ll; number = 'e'; break;
			case 'p': scale = 1000000000000000ll; number = 's'; break;
			case 'P': scale = 1000000000000000ll; number = 'S'; break;
		}

		if (number == 's' || number == 'e') {
			long long value;

			char * endptr;
			value = strtoll(optarg, &endptr, 10);
			if (endptr == optarg || value < 0) {
				setTimeNow();
				fprintf(stderr, "%s Error: invalid number %s\n", timeNow, optarg);
				exit(1);
			}

			if (number == 's') startValue = value * scale;
			else /* if (number == 'e')*/ endValue = value * scale;
		}
	}
	
	fileCount = optind - argC;
	files = argV + optind;
}



int main(int argC, char ** argV) {
	parseArgs(argC, argV);
	if (fileCount == 0) initializeSelf();
	else initializeFromFile();
	process();
	final();
	return 0;
}

