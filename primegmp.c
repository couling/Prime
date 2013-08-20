#include<stdio.h>
#include<string.h>
#include<errno.h>
#include "primegmp.h"
#include "primeshared.h"

char * prime_to_str(char * s, Prime prime) {
    Prime buffer;
    mpn_copyd(buffer,prime,PRIME_SIZE);
    mp_size_t i = PRIME_SIZE;
    while (i && buffer[i-1] == 0) --i;
    mp_size_t x = mpn_get_str(s,10,buffer,i);
    for (i = 0; i < x; ++i) s[i] += '0';
    s[i] = '\0';
    return s + x;
}



void str_to_prime(Prime prime, char * s) {
    size_t sSize = strlen(s);
    if (sSize > PRIME_STRING_SIZE) {
	int lerrno = errno;
        fprintf(stderr, "%s Error: Invalid Number (%s)\n",
            timeNow(), s);
        exitError(2, lerrno);
    }
    
    char inBuffer[PRIME_STRING_SIZE];
    
    size_t pos;
    for (pos = 0; pos < sSize; ++pos) {
        if (s[pos] > '9' || s[pos] < '0') {
            int lerrno = errno;
            fprintf(stderr, "%s Error: Invalid Number (%s)\n",
                timeNow(), s);
            exitError(2, lerrno);
        }
        inBuffer[pos] = s[pos] - '0';
    }

    mp_size_t writtenSize = mpn_set_str(prime, inBuffer, sSize, 10);
    while (writtenSize < PRIME_SIZE) {
        prime[writtenSize] = 0;
	++writtenSize;
    }
}



void prime_set_num(Prime prime, mp_limb_t in) {
    mpn_zero (prime, PRIME_SIZE);
	prime[0] = in;
}



void prime_add_num(Prime target, Prime in1, mp_limb_t in2) {
    Prime tmp;
    mpn_add_1(tmp, in1, PRIME_SIZE, in2);
	mpn_copyd(target, tmp, PRIME_SIZE);
}



void prime_add_prime(Prime target, Prime in1, Prime in2) {
    Prime tmp;
    mpn_add_n(tmp, in1, in2, PRIME_SIZE);
	mpn_copyd(target, tmp, PRIME_SIZE);
}



void prime_sub_num(Prime target, Prime in1, mp_limb_t in2) {
    Prime tmp;
    mpn_sub_n(tmp, in1, PRIME_SIZE, in2);
	mpn_copyd(target, tmp, PRIME_SIZE);
}



void prime_sub_prime(Prime target, Prime in1, Prime in2) {
    Prime tmp;
    mpn_sub_n(tmp, in1, in2, PRIME_SIZE);
	mpn_copyd(target, tmp, PRIME_SIZE);
}



void prime_left_shift(Prime target, Prime in, unsigned int count) {
    Prime tmp;
	mpn_lshift(target, in, PRIME_SIZE, count);
	mpn_copyd(target, tmp, PRIME_SIZE);
}



void prime_right_shift(Prime target, Prime in, unsigned int count) {
    Prime tmp;
	mpn_rshift(target, in, PRIME_SIZE, count);
	mpn_copyd(target, tmp, PRIME_SIZE);
}



void prime_mul(Prime target, Prime in1, Prime in2) {
    mp_limb_t result[PRIME_SIZE * 2];
    mpn_mul_n(result, in1, in2, PRIME_SIZE);
    mpn_copyd(target, result, PRIME_SIZE);
}



void prime_div_mod(Prime div, Prime mod, Prime in1, Prime in2) {
    Prime ldiv;
    Prime lmod;
    p_size_t in2Size = PRIME_SIZE;
    while (!in2[in2Size-1]&&in2Size) --in2Size;
    mpn_tdiv_qr(ldiv, lmod, 0, in1, PRIME_SIZE, in2, in2Size);
    mpn_copyd(div,ldiv,PRIME_SIZE);
    mpn_copyd(mod,lmod,PRIME_SIZE);
}



void prime_div(Prime div, Prime in1, Prime in2) {
    Prime ldiv;
    Prime lmod;
    mp_size_t in2Size = PRIME_SIZE;
    while (!in2[in2Size-1]&&in2Size) --in2Size;
    mpn_tdiv_qr(ldiv, lmod, 0, in1, PRIME_SIZE, in2, in2Size);
    mpn_copyd(div,ldiv,PRIME_SIZE);
}



void prime_mod(Prime mod, Prime in1, Prime in2) {
    Prime ldiv;
    Prime lmod;
    mp_size_t in2Size = PRIME_SIZE;
    while (!in2[in2Size-1]&&in2Size) --in2Size;
    mpn_tdiv_qr(ldiv, lmod, 0, in1, PRIME_SIZE, in2, in2Size);
    mpn_copyd(mod,lmod,PRIME_SIZE);
}



void prime_sqr(Prime target, Prime in) {
    mp_limb_t result[PRIME_SIZE * 2];
    mpn_sqr(result, in, PRIME_SIZE);
    mpn_copyd(target, result, PRIME_SIZE);
}



void prime_sqrt(Prime target, Prime in) {
}

