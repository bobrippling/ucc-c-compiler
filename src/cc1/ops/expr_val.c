#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_val.h"
#include "../out/asm.h"

const char *str_expr_val()
{
	return "val";
}

/*

-- no suffix --
[0-9]+ -> int, int, long int, long long int
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

	enum type_primitive p = type_int;
	int s = 1, set_s = 0;

	(void)stab;

	if(iv->suffix & VAL_LONG)
		p = type_long;

	if(iv->suffix & VAL_UNSIGNED)
		s = 0, set_s = 1;

	/* size checks - don't rely on libc */
	const long int_max            =         0x7fffffff;
	const long uint_max           =         0xffffffff;
	/*const unsigned long ulong_max = 0xffffffffffffffff; // FIXME: 64-bit currently*/
	const          long  long_max = 0x7fffffffffffffff;

	/* a pure intval will never be negative,
	 * since we parse -5 as (- (intval 5))
	 * so if it's negative, we have long_max
	 */

	ICW("TODO: standard conforming literal typing");

	for(;;){
		if(s){
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
						/* attempt to fit into unsigned int */
						s = 0;
						continue;
					}
					/* fits into a signed int */
					break;

				case type_long:
					if(iv->val < 0L){
						/* overflow - try unsigned long */
						s = 0;
						continue;
					}

					/* fits into signed long? */
					if((unsigned)labs(iv->val) <= (unsigned)long_max){
						/* yes */
						break;
					}
					/* doesn't fit into long, try unsigned long */
					s = 0;
					continue;
			}
			/* fine */
			break;
		}else{
			/* unsigned */
			switch(p){
				default:
					ICE("bad primitive");

				case type_int:
					if(labs(iv->val) > labs(uint_max)){
						/* attempt to fit into a signed long */
						if(!set_s)
							s = 1; /* else U specified, don't go to signed */

						p = type_long;
						continue;
					}
					/* fits into an unsigned int */
					break;

				case type_long:
					/* this is the largest type we have - TODO: check for overflows */
					break;
			}
			/* fine */
			break;
		}
	}

	EOF_WHERE(&e->where,
		e->tree_type = type_ref_new_type(type_new_primitive_signed(p, s));
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
