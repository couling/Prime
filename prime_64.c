#include <stdio.h>
#include <stdlib.h>

#include "prime_64.h"
#include "prime_shared.h"

Prime _str_to_prime(char * s) {
	Prime value;

	char * endptr;
	value = strtoll(s, &endptr, 10);
	if (*endptr || value < 0) {
		fprintf(stderr, "%s Error: invalid number %s\n", 
			timeNow(), s);
		exit(1);
	}
	
	return value;
}



void prime_to_str(char * target, Prime value) {
}
