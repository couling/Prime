#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

void exitError(int num, int errorNumber, char * str, ...) {
	flockfile(stderr);
    va_list ap;
    va_start(ap, str);
    vfprintf(stderr, str, ap);
    va_end(ap);
	fprintf(stderr, "\n");
	if (errorNumber) fprintf(stderr, "Error: %s\n", strerror(errorNumber));
	funlockfile(stderr);
	exit(num);
}

int main(int argC, char ** argV) {
    if (argC != 2) exitError(1, 0, "invalid number of arguments %d\n");
	long long toFactor, small, big, product;
	char * endptr;
	toFactor = strtoll(argV[1], &endptr, 10);
	if (*endptr || toFactor < 0) {
		exitError(1, 0, "invalid number %s", argV[1]);
	}
	
	small = big = (long long)sqrtl(toFactor);
	
	do {
		product = small * big;
		printf("%lld\t%lld\t%lld\t%lld\n", small, big, product, toFactor - product);
		small--;
		big++;
	} while (product > toFactor - small);
}
