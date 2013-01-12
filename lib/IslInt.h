#pragma once

#include <isl/int.h>

class IslInt {
private:
	isl_int val;

public:
	IslInt() {
		isl_int_init(this->val);
	}

	IslInt(signed long int val) {
		isl_int_init(this->val);
		isl_int_set_si(this->val, val);
	}

#if 0
isl_int_init(i)
isl_int_clear(i)
isl_int_set(r,i)
isl_int_set_si(r,i)
isl_int_set_gmp(r,g)
isl_int_get_gmp(i,g)
isl_int_abs(r,i)
isl_int_neg(r,i)
isl_int_swap(i,j)
isl_int_swap_or_set(i,j)
isl_int_add_ui(r,i,j)
isl_int_sub_ui(r,i,j)
isl_int_add(r,i,j)
isl_int_sub(r,i,j)
isl_int_mul(r,i,j)
isl_int_mul_ui(r,i,j)
isl_int_addmul(r,i,j)
isl_int_submul(r,i,j)
isl_int_gcd(r,i,j)
isl_int_lcm(r,i,j)
isl_int_divexact(r,i,j)
isl_int_cdiv_q(r,i,j)
isl_int_fdiv_q(r,i,j)
isl_int_fdiv_r(r,i,j)
isl_int_fdiv_q_ui(r,i,j)
isl_int_read(r,s)
isl_int_print(out,i,width)
isl_int_sgn(i)
isl_int_cmp(i,j)
isl_int_cmp_si(i,si)
isl_int_eq(i,j)
isl_int_ne(i,j)
isl_int_lt(i,j)
isl_int_le(i,j)
isl_int_gt(i,j)
isl_int_ge(i,j)
isl_int_abs_eq(i,j)
isl_int_abs_ne(i,j)
isl_int_abs_lt(i,j)
isl_int_abs_gt(i,j)
isl_int_abs_ge(i,j)
isl_int_is_zero(i)
isl_int_is_one(i)
isl_int_is_negone(i)
isl_int_is_pos(i)
isl_int_is_neg(i)
isl_int_is_nonpos(i)
isl_int_is_nonneg(i)
isl_int_is_divisible_by(i,j)
#endif

	~IslInt() {
		isl_int_clear(val);
	}
};
