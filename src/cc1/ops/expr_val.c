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
	intval *const iv = &e->bits.iv;

	int is_signed = !(iv->suffix & VAL_UNSIGNED);
	const int can_change_sign = is_signed && (iv->suffix & VAL_NON_DECIMAL);

	const int long_max_bit = 63; /* TODO */
	const int highest_bit = intval_high_bit(iv->val, e->tree_type);
	enum type_primitive p =
		iv->suffix & VAL_LLONG ? type_llong :
		iv->suffix & VAL_LONG  ? type_long  : type_int;

	/*fprintf(stderr, "----\n0x%" INTVAL_FMT_X
	      ", highest bit = %d. suff = 0x%x\n",
	      iv->val, highest_bit, iv->suffix);*/

	if(iv->val == 0){
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

	EOF_WHERE(&e->where,
		e->tree_type = type_ref_new_type(type_new_primitive_signed(p, is_signed));
	);

	(void)stab;
}

void gen_expr_val(expr *e)
{
	out_push_iv(e->tree_type, &e->bits.iv);
}

void gen_expr_str_val(expr *e)
{
	idt_printf("val: 0x%lx\n", (unsigned long)e->bits.iv.val);
}

static void const_expr_val(expr *e, consty *k)
{
	CONST_FOLD_LEAF(k);
	memcpy_safe(&k->bits.iv, &e->bits.iv);
	k->type = CONST_VAL; /* obviously vals are const */
}

void mutate_expr_val(expr *e)
{
	e->f_const_fold = const_expr_val;
}

expr *expr_new_val(int val)
{
	expr *e = expr_new_wrapper(val);
	e->bits.iv.val = val;
	return e;
}

void gen_expr_style_val(expr *e)
{
	stylef("%" INTVAL_FMT_D, e->bits.iv.val);
}
