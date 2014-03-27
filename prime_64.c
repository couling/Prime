#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "prime_shared.h"



void printValue(FILE * file, Prime value) {
	if (fprintf(file,"%lld\n", value) < 0) {
		exitError(2, errno, "Failed to write prime number (%lld)", value);
	}
}



Prime _str_to_prime(char * s) {
	Prime value;

	char * endptr;
	value = strtoll(s, &endptr, 10);
	if (*endptr || value < 0) {
		exitError(1, 0, "invalid number %s", s);
	}
	
	return value;
}



void prime_to_str(char * target, Prime value) {
	sprintf(target, "%lld", value);
}


