#include<string.h>
#include "primegmp.h"

char * prime_to_str(char * s, Prime * prime) {
    Prime buffer;
    mpn_copyd(buffer,prime,PRIME_SIZE);
    mp_size_t i = PRIME_SIZE - 1;
    while (i > 1 && buffer[i] == 0) --i;
    mp_size_t x = mpn_get_str(s,10,buffer,i);
    for (i = 0; i < x; ++i) s[i] += '0';
    return s + x;
}



void str_to_prime(Prime * prime, char * s) {
    size_t sSize = strlen(s);
    if (sSize > PRIME_STRING_SIZE) {
        fprintf(stderr, "%s Error: Invalid Number (%s)\n",
            timeNow(), s);
        exitError(2, lerrno);
    }
    
    char inBuffer[PRIME_STRING_SIZE];
    
    size_t pos;
    for (pos = sSize -1; pos; --pos) {
        if (s[pos] > '9' || s[pos] < '0') {
            fprintf(stderr, "%s Error: Invalid Number (%s)\n",
                timeNow(), s);
            exitError(2, lerrno);
        }
        inBuffer[pos] = s[pos] - '0';
    }

    mpn_set_str(prime, inBuffer, sSize, 10);
}



void prime_mul(Prime * target, Prime * in1, Prime * in2) {
    mp_limb_t result[PRIME_SIZE * 2];
    mpn_mul_n(result, in1, in2, PRIME_SIZE);
    mpn_copyd(target, result, PRIME_SIZE);
}



void prime_div_mod(Prime * div, Prime * mod, Prime * in1, Prime * in2) {
    mpn_divmod(div, mod, 0, in1, PRIME_SIZE, in2);
}



void prime_sqr(Prime * target, Prime * in) {
    mp_limb_t result[PRIME_SIZE * 2];
    mpn_sqr(result, in, PRIME_SIZE);
    mpn_copyd(target, result, PRIME_SIZE);
}



void prime_sqrt(Prime * target, Prime * in) {
}
