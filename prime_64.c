#include "prime-long-long.h"

#ifdef PRIME_BYTE_COUNT
#undef PRIME_BYTE_COUNT
#endif // PRIME_BYTE_COUNT

#define PRIME_BYTE_COUNT sizeof(long long)
#define PRIME_STRING_SIZE ( (int)( ( ( PRIME_BYTE_COUNT * 8 * 3.01 ) / 10 ) + 1 ) )

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