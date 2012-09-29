#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TOTAL_PRIMES 100000

char timeNow[100];

void setTimeNow() {
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	
	strftime (timeNow,100,"%a %b %d %H:%M:%S %Y",timeinfo);
}

int main(int argC, char ** argV) {
	int maxUsable = 0;
	int primeCount = 1;
	long long * primes = calloc(TOTAL_PRIMES,sizeof(long long));
	if (!primes) {
		setTimeNow();
		fprintf(stderr, "%s Could not allocate %d bytes for prime array.\n", timeNow, TOTAL_PRIMES);
		return 1;
	}
	primes[0] = 3;
	printf("%d\n", 2); 
	printf("%d\n", 3);
	long long value = 5; // we have to start somewhere so we might as well start on a prime
	do {
		long long nextPrimeRequired = primes[maxUsable] * primes[maxUsable];
		setTimeNow();
		fprintf(stderr, "%s Checking against %d primes - max %lld - checking value %lld\n", 
			timeNow, maxUsable+1, primes[maxUsable], value);
		while (value < nextPrimeRequired && primeCount < TOTAL_PRIMES) {
			int i=0;
			while (i<maxUsable && value % primes[i]) ++i;
			if (i == maxUsable) {
				printf("%lld\n", value);
				primes[primeCount] = value;
				++primeCount;
			}
			value += 2;
		}
		
		++maxUsable;
	} while (primeCount < TOTAL_PRIMES);

	fprintf(stderr, "%s Prime array now full with %d primes.\n", timeNow, TOTAL_PRIMES);

	do {
		long long nextPrimeRequired = primes[maxUsable] * primes[maxUsable];
		setTimeNow();
		fprintf(stderr, "%s Checking against %d primes - max %lld - checking value %lld\n", 
			timeNow, maxUsable+1, primes[maxUsable], value);
		do {
			int i=0;
			while (i<maxUsable && value % primes[i] ) ++i;
			if (i == maxUsable) printf("%lld\n", value);
			value += 2;
		} while (value < nextPrimeRequired);
		
		++maxUsable;
	} while (maxUsable < TOTAL_PRIMES);

	free(primes);

	return 0;
}

