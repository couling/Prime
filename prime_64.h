#ifndef prime_long_long_h
#define prime_long_long_h

#ifdef PRIME_BYTE_COUNT
#undef PRIME_BYTE_COUNT
#endif // PRIME_BYTE_COUNT

#define PRIME_BYTE_COUNT sizeof(long long)
#define PRIME_STRING_SIZE ( (int)( ( ( PRIME_BYTE_COUNT * 8 * 3.01 ) / 10 ) + 1 ) )


typedef long long Prime;

#define prime_set_num(target, value) target = value
#define str_to_prime(target, value) target = _str_to_prime(value)

#define prime_mul(target, value1, value2) target = value1 * value2

Prime _str_to_prime(char * s);

#endif // prime_long_long_h