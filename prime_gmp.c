#include<stdio.h>
#include<string.h>
#include<errno.h>

#include "prime_shared.h"

char * prime_to_str(char * s, Prime prime) {
    Prime buffer;
    mpn_copyd(buffer,prime,PRIME_LIMB_COUNT);
    mp_size_t i = PRIME_LIMB_COUNT;
    while (i && buffer[i-1] == 0) --i;
    mp_size_t x = mpn_get_str(s,10,buffer,i);
    for (i = 0; i < x; ++i) s[i] += '0';
    s[i] = '\0';
    return s + x;
}



void str_to_prime(Prime prime, char * s) {
    size_t sSize = strlen(s);
    if (sSize > PRIME_STRING_SIZE) exitError(1,errno,"Invalid Number (%s)\n", s);
    
    char inBuffer[PRIME_STRING_SIZE];
    
    size_t pos;
    for (pos = 0; pos < sSize; ++pos) {
        if (s[pos] > '9' || s[pos] < '0') {
            int lerrno = errno;
            exitError(1, 0,"Invalid Number (%s)", s);
        }
        inBuffer[pos] = s[pos] - '0';
    }

    mp_size_t writtenSize = mpn_set_str(prime, inBuffer, sSize, 10);
    while (writtenSize < PRIME_LIMB_COUNT) {
        prime[writtenSize] = 0;
    ++writtenSize;
    }
}



void prime_set_num(Prime prime, mp_limb_t in) {
    mpn_zero (prime, PRIME_LIMB_COUNT);
	prime[0] = in;
}



void prime_add_num(Prime target, Prime in1, mp_limb_t in2) {
    Prime tmp;
    mpn_add_1(tmp, in1, PRIME_LIMB_COUNT, in2);
    mpn_copyd(target, tmp, PRIME_LIMB_COUNT);
}



void prime_add_prime(Prime target, Prime in1, Prime in2) {
    Prime tmp;
    mpn_add_n(tmp, in1, in2, PRIME_LIMB_COUNT);
    mpn_copyd(target, tmp, PRIME_LIMB_COUNT);
}



void prime_sub_num(Prime target, Prime in1, mp_limb_t in2) {
    Prime tmp;
    mpn_sub_1(tmp, in1, PRIME_LIMB_COUNT, in2);
	mpn_copyd(target, tmp, PRIME_LIMB_COUNT);
}



void prime_sub_prime(Prime target, Prime in1, Prime in2) {
    Prime tmp;
    mpn_sub_n(tmp, in1, in2, PRIME_LIMB_COUNT);
	mpn_copyd(target, tmp, PRIME_LIMB_COUNT);
}



void prime_left_shift(Prime target, Prime in, unsigned int count) {
    Prime tmp;
	mpn_lshift(target, in, PRIME_LIMB_COUNT, count);
	mpn_copyd(target, tmp, PRIME_LIMB_COUNT);
}



void prime_right_shift(Prime target, Prime in, unsigned int count) {
    Prime tmp;
	mpn_rshift(target, in, PRIME_LIMB_COUNT, count);
	mpn_copyd(target, tmp, PRIME_LIMB_COUNT);
}



void prime_mul_prime(Prime target, Prime in1, Prime in2) {
    mp_limb_t result[PRIME_LIMB_COUNT * 2];
    mpn_mul_n(result, in1, in2, PRIME_LIMB_COUNT);
    mpn_copyd(target, result, PRIME_LIMB_COUNT);
}



void prime_mul_num(Prime target, Prime in1, mp_limb_t in2) {
    Prime result;
    mpn_mul_1(result, in1, PRIME_LIMB_COUNT, in2);
    mpn_copyd(target, result, PRIME_LIMB_COUNT);
}



void prime_div_prime(Prime div, Prime in1, Prime in2) {
    Prime ldiv;
    Prime lmod;
	mpn_zero(ldiv, PRIME_LIMB_COUNT);
    mp_size_t in2Size = PRIME_LIMB_COUNT;
    while (!in2[in2Size-1]&&in2Size) --in2Size;
    mpn_tdiv_qr(ldiv, lmod, 0, in1, PRIME_LIMB_COUNT, in2, in2Size);
    mpn_copyd(div,ldiv,PRIME_LIMB_COUNT);
}



void prime_div_num(Prime div, Prime in1, mp_limb_t in2) {
    Prime ldiv;
    Prime lmod;
	mpn_zero(ldiv, PRIME_LIMB_COUNT);
    mpn_tdiv_qr(ldiv, lmod, 0, in1, PRIME_LIMB_COUNT, &in2, 1);
	mpn_copyd(div,ldiv,PRIME_LIMB_COUNT);
}



void prime_mod_prime(Prime mod, Prime in1, Prime in2) {
    Prime ldiv;
    Prime lmod;
	mpn_zero(lmod, PRIME_LIMB_COUNT);
    mp_size_t in2Size = PRIME_LIMB_COUNT;
    while (!in2[in2Size-1] && in2Size) --in2Size;
    mpn_tdiv_qr(ldiv, lmod, 0, in1, PRIME_LIMB_COUNT, in2, in2Size);
    mpn_copyd(mod,lmod,PRIME_LIMB_COUNT);
}



void prime_mod_num(Prime mod, Prime in1, mp_limb_t in2) {
    Prime ldiv;
    Prime lmod;
    mpn_zero(lmod, PRIME_LIMB_COUNT);
    mpn_tdiv_qr(lmod, lmod, 0, in1, PRIME_LIMB_COUNT, &in2, 1);
    mpn_copyd(mod,lmod,PRIME_LIMB_COUNT);
}



void prime_sqr(Prime target, Prime in) {
    mp_limb_t result[PRIME_LIMB_COUNT * 2];
    mpn_sqr(result, in, PRIME_LIMB_COUNT);
    mpn_copyd(target, result, PRIME_LIMB_COUNT);
}



void prime_sqrt(Prime target, Prime in) {
    Prime result;
    mp_size_t inSize = PRIME_LIMB_COUNT;
	while (!in[inSize-1] && inSize) --inSize;
	mpn_zero(result, PRIME_LIMB_COUNT);
    mpn_sqrtrem (result, NULL, in, inSize);
    prime_cp(target, result);
}

