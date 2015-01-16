#ifndef prime_long_long_h
#define prime_long_long_h

#define PRIME_ARCHITECTURE Long Long Int

#define PRIME_LIMB_SIZE sizeof(long long) 
#define PRIME_LIMB_COUNT ((size_t) 1)

typedef long long Prime;

#define prime_set_num(target, value) target = value
#define prime_get_num(value) ( value )
#define str_to_prime(target, value) target = _str_to_prime(value)
Prime _str_to_prime(char * s);
void prime_to_str(char * target, Prime value);


#define prime_add_num(target, in1, in2) ( target = in1 + in2 )
#define prime_add_prime(target, in1, in2) ( target = in1 + in2 )
#define prime_sub_num(target, in1, in2) ( target = in1 - in2 )
#define prime_sub_prime(target, in1, in2) ( target = in1 - in2 )


#define prime_mul_prime(target, value1, value2) ( target = value1 * value2 )
#define prime_mul_num(target, value1, value2) ( target = value1 * value2 )
#define prime_mul_16(target, value1) ( target = value1 << 4 )
//void prime_div_mod(Prime div, Prime mod, Prime in1, Prime in2);
#define prime_div_prime(div, in1, in2) ( div = in1 / in2 )
#define prime_div_num(div, in1, in2) ( div = in1 / in2 )
#define prime_div_16(div, in1) ( div = in1 >> 4)
#define prime_mod_prime(mod, in1, in2) ( mod = in1 % in2 )
#define prime_mod_num(mod, in1, in2) ( mod = in1 % in2 )


#define prime_sqrt(target, value) target = (Prime) sqrtl((long double) value )


#define prime_gt(v1,v2) ( v1 >  v2 )
#define prime_ge(v1,v2) ( v1 >= v2 )
#define prime_lt(v1,v2) ( v1 <  v2 )
#define prime_le(v1,v2) ( v1 <= v2 )
#define prime_eq(v1,v2) ( v1 == v2 )

#define prime_is_odd(v1)  ( v1 & 1 )

#define prime_cp(target,result) ( target = result )


#endif // prime_long_long_h
