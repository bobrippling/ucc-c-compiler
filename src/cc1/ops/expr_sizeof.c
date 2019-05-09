#include <string.h>
#include <assert.h>

#include "ops.h"
#include "expr_sizeof.h"
#include "../sue.h"
#include "../out/asm.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../vla.h"

#define SIZEOF_WHAT(e) ((e)->expr ? (e)->expr->tree_type : (e)->bits.size_of.of_type)
#define SIZEOF_SIZE(e)  (e)->bits.size_of.sz

#define NEED_RUNTIME_SIZEOF(ty) !!type_is_vla((ty), VLA_ANY_DIMENSION)

#define sizeof_this tref

static const char *sizeof_what(enum what_of wo)
{
	switch(wo){
		case what_typeof:
			return "typeof";
		case what_sizeof:
			return "sizeof";
		case what_alignof:
			return "alignof";
	}

	return NULL;
}

type *expr_sizeof_type(expr *e)
{
	return SIZEOF_WHAT(e);
}

const char *str_expr_sizeof(void)
{
	return "sizeof/typeof/alignof";
}

void fold_expr_sizeof(expr *e, symtable *stab)
{
	type *chosen;

	if(e->expr)
		fold_expr_nodecay(e->expr, stab);
	else
		fold_type(e->bits.size_of.of_type, stab);

	chosen = SIZEOF_WHAT(e);

	if(fold_check_expr(e->expr,
			FOLD_CHK_NO_BITFIELD
			| (e->what_of == what_typeof || e->what_of == what_sizeof
					? FOLD_CHK_ALLOW_VOID
					: 0),
			sizeof_what(e->what_of)))
	{
		e->tree_type = type_nav_btype(cc1_type_nav, type_int);
		return;
	}

	switch(e->what_of){
		case what_typeof:
			e->tree_type = chosen;
			break;

		case what_sizeof:
		{
			/* check for sizeof array parameter */
			if(type_is_decayed_array(chosen)){
				char ar_buf[TYPE_STATIC_BUFSIZ];

				cc1_warn_at(&e->where,
						sizeof_decayed,
						"array-argument evaluates to sizeof(%s), not sizeof(%s)",
						type_to_str(chosen),
						type_to_str_r_show_decayed(ar_buf, chosen));
			}
		} /* fall */

		case what_alignof:
		{
			int set = 0; /* need this, since .bits can't be relied upon to be 0 */
			int vla = NEED_RUNTIME_SIZEOF(chosen);

			if(!type_is_complete(chosen)){
				if(type_is_void(chosen))
					cc1_warn_at(&e->where, sizeof_void_or_func, "%s() on void type", sizeof_what(e->what_of));
				else
					die_at(&e->where, "%s incomplete type %s", sizeof_what(e->what_of), type_to_str(chosen));
			}

			if(type_is(chosen, type_func))
				cc1_warn_at(&e->where, sizeof_void_or_func, "%s() on function type", sizeof_what(e->what_of));

			if((e->what_of == what_alignof || vla) && e->expr){
				decl *d = expr_to_declref(e->expr, NULL);

				if(d){
					if(e->what_of == what_alignof){
						SIZEOF_SIZE(e) = decl_align(d);
					}else{
						assert(vla);
						e->bits.size_of.vm = d;
					}
					set = 1;
				}
			}

			if(!set){
				if(!vla){
					SIZEOF_SIZE(e) = (e->what_of == what_sizeof
							? type_size : type_align)(chosen, &e->where);
				}
			}

			/* size_t */
			e->tree_type = type_nav_btype(cc1_type_nav, type_ulong);
			break;
		}
	}
}

static void const_expr_sizeof(expr *e, consty *k)
{
	if(NEED_RUNTIME_SIZEOF(SIZEOF_WHAT(e))){
		CONST_FOLD_NO(k, e);
		return;
	}

	CONST_FOLD_LEAF(k);
	k->bits.num.val.i = SIZEOF_SIZE(e);
	k->bits.num.suffix = VAL_UNSIGNED | VAL_LONG;
	k->type = CONST_NUM;
}

const out_val *gen_expr_sizeof(const expr *e, out_ctx *octx)
{
	type *ty = SIZEOF_WHAT(e);

	if(NEED_RUNTIME_SIZEOF(ty)){
		/* if it's an expression, we want eval, e.g.
		 *   short ar[1][f()];
		 *   return sizeof ar[g()]; // want f() and g()
		 *
		 * C11 6.5.3.4 p2
		 * The sizeof operator yields the size (in bytes) of its operand, which may
		 * be an expression or the parenthesized name of a type. The size is
		 * determined from the type of the operand. The result is an integer.
		 *
		 * ###
		 * If the type of the operand is a variable length array type, the operand
		 * is evaluated;
		 * ###
		 *
		 * otherwise, the operand is not evaluated and the result is an integer
		 * constant.
		 *
		 *
		 * C11 6.7.6.2 p5
		 *
		 * If the size is an expression that is not an integer constant expression:
		 * if it occurs in a declaration at function prototype scope, it is treated
		 * as if it were replaced by *; otherwise, each time it is evaluated it
		 * shall have a value greater than zero. The size of each instance of a
		 * variable length array type does not change during its lifetime. Where a
		 * size expression is part of the operand of a sizeof operator and changing
		 * the value of the size expression would not affect the result of the
		 * operator, it is unspecified whether or not the size expression is
		 * evaluated.
		 *
		 * - currently we always evaluate it, the backend may discard things
		 * like integer constants
		 */
		if(e->expr)
			out_val_consume(octx, gen_expr(e->expr, octx));

		return vla_size(ty, octx);
	}

	return out_new_l(octx, e->tree_type, SIZEOF_SIZE(e));
}

void dump_expr_sizeof(const expr *e, dump *ctx)
{
	dump_desc_expr_newline(ctx, sizeof_what(e->what_of), e, 0);

	if(e->expr){
		dump_printf(ctx, "\n");
		dump_inc(ctx);
		dump_expr(e->expr, ctx);
		dump_dec(ctx);
	}else{
		dump_printf(ctx, " %s\n", type_to_str(e->bits.size_of.of_type));
	}
}

void mutate_expr_sizeof(expr *e)
{
	e->f_const_fold = const_expr_sizeof;
	e->bits.size_of.vm = NULL;
}

expr *expr_new_sizeof_type(type *t, enum what_of what_of)
{
	expr *e = expr_new_wrapper(sizeof);
	e->bits.size_of.of_type = t;
	e->what_of = what_of;
	return e;
}

expr *expr_new_sizeof_expr(expr *sizeof_this, enum what_of what_of)
{
	expr *e = expr_new_wrapper(sizeof);
	e->expr = sizeof_this;
	e->what_of = what_of;
	return e;
}

const out_val *gen_expr_style_sizeof(const expr *e, out_ctx *octx)
{
	stylef("%s(", sizeof_what(e->what_of));

	if(e->expr)
		IGNORE_PRINTGEN(gen_expr(e->expr, octx));
	else
		stylef("%s", type_to_str(e->bits.size_of.of_type));

	stylef(")");

	UNUSED_OCTX();
}
