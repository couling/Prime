#ifndef prime_long_long_h
#define prime_long_long_h

typedef long long Prime;

void printValue(FILE * file, Prime value);
#define prime_set_num(target, value) target = value
#define prime_get_num(value) ( value )
#define str_to_prime(target, value) target = _str_to_prime(value)
Prime _str_to_prime(char * s);
void prime_to_str(char * target, Prime value);


#define prime_add_num(target, in1, in2) ( target = in1 + in2 )
#define prime_add_prime(target, in1, in2) ( target = in1 + in2 )
#define prime_sub_num(target, in1, in2) ( target = in1 - in2 )
#define prime_sub_prime(target, in1, in2) ( target = in1 - in2 )


#define prime_mul_prime(target, value1, value2) target = value1 * value2
#define prime_mul_num(target, value1, value2) target = value1 * value2
//void prime_div_mod(Prime div, Prime mod, Prime in1, Prime in2);
#define prime_div_prime(div, in1, in2) ( div = in1 / in2 )
#define prime_div_num(div, in1, in2) ( div = in1 / in2 )
#define prime_mod_prime(mod, in1, in2) ( mod = in1 % in2 )
#define prime_mod_num(mod, in1, in2) ( mod = in1 % in2 )


#define prime_sqrt(target, value) target = (Prime) sqrtl((long double) value )


#define prime_gt(v1,v2) ( v1 >  v2 )
#define prime_ge(v1,v2) ( v1 >= v2 )
#define prime_lt(v1,v2) ( v1 <  v2 )
#define prime_le(v1,v2) ( v1 <= v2 )
#define prime_eq(v1,v2) ( v1 == v2 )

#define prime_gt_zero(v1) ( v1 > 0 )
#define prime_lt_zero(v1) ( v1 < 0 )

#define prime_is_odd(v1)  ( v1 & 1 )

#define prime_cp(target,result) ( target = result )


#define prime_init()

#define prime_1 1
#define prime_2 2

#endif // prime_long_long_h
