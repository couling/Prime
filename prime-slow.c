/* 

Copyright Philip Couling
All rights reserved.

This is an early version of prime finding.  It's pretty slow.
It uses division - dividing each potential new prime by those primes smaller
than the number's square root.

This was obsoleted by bitprime.c

*/
#define _POSIX_C_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <getopt.h>

#include "shared.h"
#include "prime_shared.h"

#define ALLOC_UNIT 128;

int primeCount;
int maxUsable;
int primesAllocated;
Prime * primes;


void initializeSelf(FILE * file) {
    if (!silent) stdLog("Running Self initialisation for %lld (inc) to %lld (ex)", startValue, endValue);

    primesAllocated = ALLOC_UNIT;
    primes = mallocSafe(primesAllocated * sizeof(long long));

    // To bootstrap the process we hard code the first three primes
    if (startValue <= 2 && endValue > 2) fprintf(file, "%lld\n", 2ll);
    if (startValue <= 3 && endValue > 3) fprintf(file, "%lld\n", 3ll);
    if (startValue <= 3 && endValue > 3) fprintf(file, "%lld\n", 5ll);
    
    primes[0] = 3ll; 
    primes[1] = 5ll;
    primeCount=2;
    maxUsable=1;
    long long value = 7ll;
    while (1){
        long long nextPrimeRequired = primes[maxUsable] * primes[maxUsable];
        if (!silent) stdLog("Checking against %d primes - max %lld - checking value %lld",
            maxUsable, primes[maxUsable-1], value);
        while (value < nextPrimeRequired ) {
            int i=0;
            while (i<maxUsable && value % primes[i]) ++i;
            if (i == maxUsable) {
                if (primeCount == primesAllocated) {
                    primesAllocated += ALLOC_UNIT;
                    primes = reallocSafe(primes, primesAllocated * sizeof(long long));
                }
                primes[primeCount] = value;
                ++primeCount;
                if (value >= startValue) fprintf(file, "%lld\n", value);
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
        if (!silent) stdLog("All primes have now been discovered between %lld (inc) and %lld (ex)", startValue, value);
        startValue = value;
    }
    else {
        if (!(startValue & 1)) ++startValue;
        while (primes[maxUsable] * primes[maxUsable] < startValue) ++maxUsable;
    }

    if (!silent) stdLog("Prime array now full with %d primes", primeCount);

}


void process(FILE * file) {
    if (!silent) stdLog("Running process for %lld (inc) to %lld (ex)", startValue, endValue);
    long long value = startValue;
    while (value < endValue) {
        long long nextPrimeRequired = primes[maxUsable] * primes[maxUsable];
        if (!silent) stdLog("Checking against %d primes - max %lld - checking value %lld",
             maxUsable, primes[maxUsable-1], value);
        while (value < nextPrimeRequired) {
            if (value >= endValue) break;
            int i=0;
            while (i<maxUsable && value % primes[i] ) ++i;
            if (i == maxUsable) fprintf(file, "%lld\n", value);
            value += 2;
        }
        // if this dropped out the loop due to nextPrimeRequred, we know that this value
        // can be factored (next prime required is the square of a prime).
        value += 2;
        ++maxUsable;
    }

    if (!silent) stdLog("All primes have now been discovered between %lld (inc) and %lld (ex)",
        startValue, endValue);

}



void final() {
    free(primes);
}



int main(int argC, char ** argV) {
    parseArgs(argC, argV);
    
    if (initFileName != NULL) {
        exitError(1,0,"Can not initialized from file; feature not implemented");
    }
    
    FILE * theFile;
    if (useStdout) theFile = stdout;
    else theFile = fdopen(openFileForPrime(startValue, endValue), "w");
    
    initializeSelf(theFile);
    process(theFile);
    free(primes);
    fclose(theFile);
    return 0;
}

