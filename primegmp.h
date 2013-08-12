#ifndef PRIME_GMP_H
#define PRIME_GMP_H

#include <gmp.h>

#ifndef PRIME_BYTE_COUNT
#define PRIME_BYTE_COUNT 16
#endif // PRIME_BYTE_COUNT

#define PRIME_SIZE ( PRIME_BYTE_COUNT / sizeof(mp_limb_t) )
#define PRIME_STRING_SIZE (int)( ( ( PRIME_BYTE_COUNT * 8 * 3.01 ) / 10 ) + 1 )

// gmp
typedef mp_limb_t Prime[PRIME_SIZE];

char * prime_to_str(char * s, Prime prime);
void str_to_prime(Prime prime, char * s);

void prime_mul(Prime target, Prime in1, Prime in2);
void prime_div_mod(Prime div, Prime mod, Prime in1, Prime in2);
void prime_div(Prime div, Prime in1, Prime in2);
void prime_mod(Prime mod, Prime in1, Prime in2);
void prime_sqr(Prime target, Prime in);
void prime_sqrt(Prime target, Prime in); 

#define prime_gt(v1,v2) ( mpn_cmp(v1,v2,PRIME_SIZE) >  0 )
#define prime_ge(v1,v2) ( mpn_cmp(v1,v2,PRIME_SIZE) >= 0 )
#define prime_lt(v1,v2) ( mpn_cmp(v1,v2,PRIME_SIZE) <  0 )
#define prime_le(v1,v2) ( mpn_cmp(v1,v2,PRIME_SIZE) <= 0 )
#define prime_cp(target,result) mpn_copyd(target,result,PRIME_SIZE)

#endif // PRIME_GMP_H
