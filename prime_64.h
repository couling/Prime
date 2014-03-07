#ifndef prime_long_long_h
#define prime_long_long_h

#include <math.h>

#ifdef PRIME_BYTE_COUNT
#undef PRIME_BYTE_COUNT
#endif // PRIME_BYTE_COUNT

#define PRIME_BYTE_COUNT sizeof(long long)
#define PRIME_STRING_SIZE ( (int)( ( ( PRIME_BYTE_COUNT * 8 * 3 ) / 10 ) + 3 ) )


typedef long long Prime;
typedef char PrimeString[PRIME_STRING_SIZE];

void printValue(FILE * file, Prime value);
#define prime_set_num(target, value) target = value
#define str_to_prime(target, value) target = _str_to_prime(value)
Prime _str_to_prime(char * s);
void prime_to_str(char * target, Prime value);

#define prime_mul(target, value1, value2) target = value1 * value2
#define prime_sqrt(target, value) target = (Prime) sqrtl((long double) value )

#define prime_init()

#define prime_1 1
#define prime_2 2

#endif // prime_long_long_h
