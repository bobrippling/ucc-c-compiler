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
	const long  int_max  =         0x7fffffff;
	const long  uint_max =         0xffffffff;
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
	const long  long_max = UCC_LONG_MAX;
	const unsigned long ulong_max = UCC_ULONG_MAX;

	intval *const iv = &e->bits.iv;

	enum type_primitive p =
		iv->suffix & VAL_LLONG ? type_llong :
		iv->suffix & VAL_LONG  ? type_long  : type_int;

	int is_signed         = !(iv->suffix & VAL_UNSIGNED);
	const int change_sign = is_signed && (iv->suffix & VAL_NON_DECIMAL);

	(void)stab;

	/* a pure intval will never be negative,
	 * since we parse -5 as (- (intval 5))
	 * so if it's negative, we have long_max
	 */

	for(;;){
		if(is_signed){
			switch(p){
				default:
					ICE("bad primitive");

				case type_int:
					if(iv->val < 0L){
						/* overflow - try signed long */
						p = type_long;
						continue;
					}

					if(labs(iv->val) > labs(int_max)){
						/* doesn't fit into a signed int */
						if(change_sign)
							/* attempt to fit into unsigned int */
							is_signed = 0;
						else
							/* attempt to fit into signed long */
							p = type_long;

						continue;
					}
					/* fits into a signed int */
					break;

				case type_long:
					if(iv->val < 0L){
						/* overflow - try long long */
						p = type_llong;
						continue;
					}

					/* fits into signed long? */
					if((unsigned)labs(iv->val) <= (unsigned)long_max){
						/* yes */
						break;
					}

					/* doesn't fit into a signed long */
					if(change_sign){
						/* attempt to fit into unsigned long */
						is_signed = 0;
						continue;
					}else{
						/* attempt to fit into long long */
						p = type_llong;
						/* fall */
					}

				case type_llong:
					if(iv->val < 0LL){
						/* doesn't fit into long long, try unsigned long long */
						if(change_sign){
							is_signed = 0;
						}else{
							WARN_AT(&e->where, "integer constant too large");
							break; /* tough */
						}
					}
					/* fits */
					break;
			}
			/* fine */
			break;
		}else{
			/* unsigned */
			switch(p){
				unsigned long long u_max;
				enum type_primitive target_prim;

				default:
					ICE("bad primitive");

				case type_int:
					u_max = uint_max;
					target_prim = type_long;
					goto ucheck;

				case type_long:
					u_max = ulong_max;
					target_prim = type_llong;
					/* fall */
ucheck:
					if(llabs(iv->val) > llabs(u_max)){
						if(change_sign){
							is_signed = 1; /* attempt to fit into a signed long long */
							p = (p == type_int ? type_long : type_llong);
						}else{
							p = target_prim; /* else go to unsigned higher */
						}
						continue;
					}
					/* fits into an where we are */
					break;

				case type_llong:
					/* this is the largest type we have - TODO: check for overflows */
					break;
			}
			/* fine */
			break;
		}
	}

	EOF_WHERE(&e->where,
		e->tree_type = type_ref_new_type(type_new_primitive_signed(p, is_signed));
	);
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
