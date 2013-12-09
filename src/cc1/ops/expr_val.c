#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "ops.h"
#include "expr_val.h"
#include "../out/asm.h"

const char *str_expr_val()
{
	return "val";
}

/*
-- no suffix --
[0-9]+ -> int, long int, long long int
oct|hex -> int, unsigned int, long int, unsigned long int, long long int, unsigned long long int

-- u suffix --
decimal/octal/hexadecimal [Uu]suffix -> unsigned int, unsigned long int, unsigned long long int

-- l suffix --
decimal [Ll] suffix -> long int, long long int
octal/hexadecimal [Ll] suffix -> long int, unsigned long int, long long int, unsigned long long int

-- ul suffix --
[uU][lL] suffix -> unsigned long int, unsigned long long int



-- ll suffix (unsupported) --
decimal (ll|LL) suffix ->	long long int
oct/hex (ll|LL) suffix -> long long int, unsigned long long int

-- llu suffix (unsupported) --
(ll|LL)[Uu] suffix -> unsigned long long int
*/

void fold_expr_val(expr *e, symtable *stab)
{
	numeric *const num = &e->bits.num;

	int is_signed = !(num->suffix & VAL_UNSIGNED);
	const int can_change_sign = is_signed && (num->suffix & VAL_NON_DECIMAL);

	const int long_max_bit = 63; /* TODO */
	const int highest_bit = integral_high_bit(num->val.i, e->tree_type);
	enum type_primitive p =
		num->suffix & VAL_LLONG ? type_llong :
		num->suffix & VAL_LONG  ? type_long  : type_int;

	/*fprintf(stderr, "----\n0x%" NUMERIC_FMT_X
	      ", highest bit = %d. suff = 0x%x\n",
	      num->val.i, highest_bit, num->suffix);*/

	/* just bail for floats for now, apart from truncating it */
	if(num->suffix & VAL_FLOATING){
		is_signed = 1;
		/**/ if(num->suffix & VAL_FLOAT)
			p = type_float, num->val.f = (float)num->val.f;
		else if(num->suffix & VAL_DOUBLE)
			p = type_double, num->val.f = (double)num->val.f;
		else if(num->suffix & VAL_LDOUBLE)
			p = type_ldouble, num->val.f = (long double)num->val.f;
		else
			ICE("floating?");

		goto chosen;
	}

	if(num->val.i == 0){
		assert(highest_bit == -1);
		goto chosen;
	}else{
		assert(highest_bit != -1);
	}

	/* can we have a signed int? */
	if(p <= type_int && highest_bit < 31){
		/* attempt signed */
		if(can_change_sign)
			is_signed = 1;

		p = type_int;
		goto chosen;
	}

	/* uint? */
	if(p <= type_int && highest_bit == 31 && (can_change_sign || !is_signed)){
		is_signed = 0;
		p = type_int;
		goto chosen;
	}

	/* long? - only chose long if given by suffix::L */
	if(p <= type_long && highest_bit < long_max_bit){
		/* attempt signed */
		if(can_change_sign)
			is_signed = 1;

		p = type_long;
		goto chosen;
	}

	/* ulong? */
	if(p <= type_long && highest_bit == long_max_bit && (!is_signed || can_change_sign)){
		is_signed = 0;
		p = type_long;
		goto chosen;
	}

	/* long long? */
	if(p <= type_llong && highest_bit < 63){
		/* attempt signed */
		if(can_change_sign)
			is_signed = 1;

		p = type_llong;
		goto chosen;
	}

	/* ull */
	if(p <= type_llong && highest_bit == 63){
		/* we get here if we're forcing it to ull,
		 * not if the user says, so we can warn unconditionally */
		if(is_signed){
			warn_at(&e->where, "integer constant is so large it is unsigned");
			is_signed = 0;
		}
		p = type_llong;
	}else{
		/* we stick with what we started with */
	}

chosen:
	/*
	fprintf(stderr, "%s -> %ssigned %s\n",
			where_str(&e->where),
			is_signed ? "" : "un",
			type_primitive_to_str(p)); */

	if(is_signed)
		num->suffix &= ~VAL_UNSIGNED;
	else
		num->suffix |= VAL_UNSIGNED;

	if(!is_signed)
		p = TYPE_PRIMITIVE_TO_UNSIGNED(p);

	EOF_WHERE(&e->where,
		e->tree_type = type_ref_new_type(type_new_primitive(p));
	);

	(void)stab;
}

void gen_expr_val(expr *e)
{
	out_push_num(e->tree_type, &e->bits.num);
}

void gen_expr_str_val(expr *e)
{
	idt_printf("val.i: 0x%lx\n", (unsigned long)e->bits.num.val.i);
}

static void const_expr_val(expr *e, consty *k)
{
	CONST_FOLD_LEAF(k);
	memcpy_safe(&k->bits.num, &e->bits.num);
	k->type = CONST_NUM; /* obviously vals are const */
}

void mutate_expr_val(expr *e)
{
	e->f_const_fold = const_expr_val;
}

expr *expr_new_val(int val)
{
	expr *e = expr_new_wrapper(val);
	e->bits.num.val.i = val;
	return e;
}

expr *expr_new_numeric(numeric *num)
{
	expr *e = expr_new_val(0);
	memcpy_safe(&e->bits.num, num);
	return e;
}

void gen_expr_style_val(expr *e)
{
	if(K_FLOATING(e->bits.num))
		stylef("%" NUMERIC_FMT_LD, e->bits.num.val.f);
	else
		stylef("%" NUMERIC_FMT_D, e->bits.num.val.i);
}
