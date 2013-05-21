#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_val.h"
#include "../out/asm.h"
#include "../../as_cfg.h" /* m32 */

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
	/* size checks - don't rely on libc */
#ifndef UCC_M32
#  error "need to know if we're -m32 or not"
#endif
#if UCC_M32
#  define UCC_LONG_MAX  int_max
#  define UCC_ULONG_MAX uint_max
#else
#  define UCC_LONG_MAX  0x7fffffffffffffff
#  define UCC_ULONG_MAX 0xffffffffffffffff
#endif
	const struct promo_t
	{
		int sgned;
		enum type_primitive prim;
		unsigned long long max;
	} promo_tbl[] = {
		{ 1, type_int, 0x7fffffff },
		{ 0, type_int, 0xffffffff },
		{ 1, type_long,  UCC_LONG_MAX  },
		{ 0, type_long,  UCC_ULONG_MAX },
		{ 1, type_llong, 0x7fffffffffffffff },
		{ 0, type_llong, 0xffffffffffffffff },
		{ -1, 0, 0 }
	};

	intval *const iv = &e->bits.iv;

	enum type_primitive p =
		iv->suffix & VAL_LLONG ? type_llong :
		iv->suffix & VAL_LONG  ? type_long  : type_int;

	int is_signed         = !(iv->suffix & VAL_UNSIGNED);
	const int change_sign = is_signed && (iv->suffix & VAL_NON_DECIMAL);
	int i;
	typedef unsigned long long ull_t;

	/* a pure intval will never be negative,
	 * since we parse -5 as (- (intval 5))
	 * so if it's negative, we have long_max
	 */
	for(i = 0; promo_tbl[i].sgned != -1; i++){
		if(!change_sign && promo_tbl[i].sgned != is_signed)
			continue;

		/* check the signed limit */
		if(promo_tbl[i].sgned){
			if((signed long long)(iv->val & promo_tbl[i].max) < 0)
				continue;
		}
		/* check the unsigned limit */
		if((ull_t)iv->val <= promo_tbl[i].max){
			/* done */
			is_signed = promo_tbl[i].sgned;
			p = promo_tbl[i].prim;
			break;
		}
	}

	if(promo_tbl[i].sgned != -1){
		WARN_AT(&e->where, "integer literal too large for any type");
		p = type_llong;
		is_signed = 0;
	}

	fprintf(stderr, "%s -> %ssigned %s\n",
			where_str(&e->where),
			is_signed ? "" : "un",
			type_primitive_to_str(p));

	EOF_WHERE(&e->where,
		e->tree_type = type_ref_new_type(type_new_primitive_signed(p, is_signed));
	);

	(void)stab;
}

void gen_expr_val(expr *e, symtable *stab)
{
	(void)stab;

	out_push_iv(e->tree_type, &e->bits.iv);
}

void gen_expr_str_val(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("val: 0x%lx\n", (unsigned long)e->bits.iv.val);
}

void const_expr_val(expr *e, consty *k)
{
	memset(k, 0, sizeof *k);
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

void gen_expr_style_val(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
