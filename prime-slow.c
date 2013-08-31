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

#include "prime_shared.h"
#include "prime_64.h"

#define ALLOC_UNIT 128;

int primeCount;
int maxUsable;
int primesAllocated;
long long * primes;
long long startValue = 0;
long long endValue = 1000000ll;

char ** files;
int fileCount;



void initializeSelf() {
	fprintf(stderr, "%s Running Self initialisation for %lld (inc) to %lld (ex)\n", 
		timeNow(), startValue, endValue);

	primesAllocated = ALLOC_UNIT;
	primes = malloc(primesAllocated * sizeof(long long));
	if (!primes) {
		fprintf(stderr, "%s Could not allocate %d bytes for prime array\n", 
			timeNow(), (int) (primesAllocated * sizeof(long long)));
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
        fprintf(stderr, "%s Checking against %d primes - max %lld - checking value %lld\n",
            timeNow(), maxUsable, primes[maxUsable-1], value);
        while (value < nextPrimeRequired ) {
            int i=0;
            while (i<maxUsable && value % primes[i]) ++i;
            if (i == maxUsable) {
				if (primeCount == primesAllocated) {
					primesAllocated += ALLOC_UNIT;
					primes = realloc(primes, primesAllocated * sizeof(long long));
					if (!primes) {
						fprintf(stderr, "%s Could not (re)allocate %d bytes for prime array\n", 
							timeNow(), (int) (primesAllocated * sizeof(long long)));
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

	if (startValue < value) {
		fprintf(stderr, "%s All primes have now been discovered between %lld (inc) and %lld (ex)\n",
			timeNow(), startValue, value);
		startValue = value;
	}
	else {
		if (!(startValue & 1)) ++startValue;
		while (primes[maxUsable] * primes[maxUsable] < startValue) ++maxUsable;
	}

	fprintf(stderr,"%s Prime array now full with %d primes\n", timeNow(), primeCount);

}



void initializeFromFile() {

}



void process() {
	fprintf(stderr, "%s Running process for %lld (inc) to %lld (ex)\n", timeNow(), startValue, endValue);
	long long value = startValue;
    while (value < endValue) {
        long long nextPrimeRequired = primes[maxUsable] * primes[maxUsable];
        fprintf(stderr, "%s Checking against %d primes - max %lld - checking value %lld\n",
            timeNow(), maxUsable, primes[maxUsable-1], value);
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

    fprintf(stderr, "%s All primes have now been discovered between %lld (inc) and %lld (ex)\n",
        timeNow(), startValue, endValue);

}



void final() {
	free(primes);
}



int main(int argC, char ** argV) {
	parseArgs(argC, argV);
	str_to_prime(startValue, startValueString);
	str_to_prime(endValue, endValueString);
	if (fileCount == 0) initializeSelf();
	else initializeFromFile();
	process();
	final();
	return 0;
}

