#ifndef PRIME_GMP_H
#define PRIME_GMP_H

#include <gmp.h>

// gmp
#define PRIME_LIMB_COUNT ( PRIME_SIZE / sizeof(mp_limb_t) / 8 )
typedef mp_limb_t Prime[PRIME_LIMB_COUNT];

char * prime_to_str(char * s, Prime prime);
void str_to_prime(Prime prime, char * s);
void prime_set_num(Prime prime, mp_limb_t in);
#define prime_get_num(prime) (prime[0])

void prime_add_num(Prime target, Prime in1, mp_limb_t in2);
void prime_add_prime(Prime target, Prime in1, Prime in2);
void prime_sub_num(Prime target, Prime in1, mp_limb_t in2);
void prime_sub_prime(Prime target, Prime in1, Prime in2);

void prime_left_shift(Prime target, Prime in, unsigned int count);
void prime_right_shift(Prime target, Prime in, unsigned int count);

void prime_mul_prime(Prime target, Prime in1, Prime in2);
void prime_mul_num(Prime target, Prime in1, mp_limb_t in2);
#define prime_mul_16(target, in1) mpn_lshift(target, in1, PRIME_LIMB_COUNT, 4)
//void prime_div_mod(Prime div, Prime mod, Prime in1, Prime in2);
void prime_div_prime(Prime div, Prime in1, Prime in2);
void prime_div_num(Prime div, Prime in1, mp_limb_t in2);
#define prime_div_16(div, in1) mpn_rshift(div, in1, PRIME_LIMB_COUNT, 4)
void prime_mod_prime(Prime mod, Prime in1, Prime in2);
void prime_mod_num(Prime mod, Prime in1, mp_limb_t in2);

void prime_sqr(Prime target, Prime in);
void prime_sqrt(Prime target, Prime in); 

#define prime_gt(v1,v2) ( mpn_cmp(v1,v2,PRIME_LIMB_COUNT) >  0 )
#define prime_ge(v1,v2) ( mpn_cmp(v1,v2,PRIME_LIMB_COUNT) >= 0 )
#define prime_lt(v1,v2) ( mpn_cmp(v1,v2,PRIME_LIMB_COUNT) <  0 )
#define prime_le(v1,v2) ( mpn_cmp(v1,v2,PRIME_LIMB_COUNT) <= 0 )
#define prime_eq(v1,v2) ( mpn_cmp(v1,v2,PRIME_LIMB_COUNT) == 0 )

#define prime_is_odd(v1)  ( v1 [0] & 1 )

#define prime_cp(target,result) mpn_copyd(target,result,PRIME_LIMB_COUNT)

#endif // PRIME_GMP_H
