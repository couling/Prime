#include <stdio.h>
#include <stdlib.h>

#define TOTAL_PRIMES 100000

int main(int argC, char ** argV) {
	int maxUsable = 0;
	int primeCount = 1;
	long long * primes = calloc(TOTAL_PRIMES,sizeof(long long));
	if (!primes) {
		fprintf(stderr, "Could not allocate %d bytes for prime array.\n", TOTAL_PRIMES);
		return 1;
	}
	primes[0] = 3;

	printf("%d\n", 2);
	printf("%d\n", 3);
	long long value = 5; // we have to start somewhere so we might as well start on a prime
	while (maxUsable < (TOTAL_PRIMES - 1)) {
		int i;
		for (i=0; i<maxUsable; i++) {
			//fprintf(stderr, "checking %d mod %d\n", value, (primes[i]));
			if (!(value % primes[i])) {
				break;
			}
		}
		if (i == maxUsable) {
			if (primeCount < TOTAL_PRIMES) {
				primes[primeCount] = value;
				primeCount++;
			}
			printf("%lld\n", value);
		}

		value += 2;
		if (primes[maxUsable] * primes[maxUsable] <= value) {
			fprintf(stderr, "Checking against %d primes - max %lld - checking value %lld\n", maxUsable+1, primes[maxUsable], value);
			maxUsable++;
		}
		
	}

	free(primes);

	return 0;
}

