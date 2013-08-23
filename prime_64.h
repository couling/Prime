#ifndef prime_long_long_h
#define prime_long_long_h

typedef long long Prime;

#define prime_set_num(target, value) target = value
#define str_to_prime(target, value) target = _str_to_prime(value)

#define prime_mul(target, value1, value2) target = value1 * value2

Prime _str_to_prime(char * s);

#endif // prime_long_long_h