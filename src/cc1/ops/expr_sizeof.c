#include <string.h>

#include "ops.h"
#include "expr_sizeof.h"
#include "../sue.h"
#include "../out/asm.h"
#include "../type_is.h"
#include "../type_nav.h"

#define SIZEOF_WHAT(e) ((e)->expr ? (e)->expr->tree_type : (e)->bits.size_of.of_type)
#define SIZEOF_SIZE(e)  (e)->bits.size_of.sz

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

const char *str_expr_sizeof()
{
	return "sizeof/typeof/alignof";
}

void fold_expr_sizeof(expr *e, symtable *stab)
{
	type *chosen;

	if(e->expr)
		fold_expr_no_decay(e->expr, stab);
	else
		fold_type(e->bits.size_of.of_type, stab);

	chosen = SIZEOF_WHAT(e);

	fold_check_expr(e->expr,
			FOLD_CHK_NO_BITFIELD
			| (e->what_of == what_typeof
					? FOLD_CHK_ALLOW_VOID
					: 0),
			sizeof_what(e->what_of));

	switch(e->what_of){
		case what_typeof:
			e->tree_type = chosen;
			break;

		case what_sizeof:
		{
			/* check for sizeof array parameter */
			if(type_is_decayed_array(chosen)){
				char ar_buf[TYPE_STATIC_BUFSIZ];

				warn_at(&e->where,
						"array-argument evaluates to sizeof(%s), not sizeof(%s)",
						type_to_str(chosen),
						type_to_str_r_show_decayed(ar_buf, chosen));
			}
		} /* fall */

		case what_alignof:
		{
			struct_union_enum_st *sue;
			int set = 0; /* need this, since .bits can't be relied upon to be 0 */

			if(!type_is_complete(chosen))
				die_at(&e->where, "sizeof incomplete type %s", type_to_str(chosen));

			if((sue = type_is_s_or_u(chosen)) && !sue_complete(sue))
				die_at(&e->where, "sizeof %s", type_to_str(chosen));

			if(e->what_of == what_alignof && e->expr){
				decl *d = NULL;

				if(expr_kind(e->expr, identifier))
					d = e->expr->bits.ident.bits.ident.sym->decl;
				else if(expr_kind(e->expr, struct))
					d = e->expr->bits.struct_mem.d;

				if(d)
					SIZEOF_SIZE(e) = decl_align(d), set = 1;
			}

			if(!set)
				SIZEOF_SIZE(e) = (e->what_of == what_sizeof
						? type_size : type_align)(
							SIZEOF_WHAT(e), &e->where);

			/* size_t */
			e->tree_type = type_nav_btype(cc1_type_nav, type_ulong);
			break;
		}
	}
}

static void const_expr_sizeof(expr *e, consty *k)
{
	UCC_ASSERT(e->tree_type, "const_fold on sizeof before fold");
	CONST_FOLD_LEAF(k);
	k->bits.num.val.i = SIZEOF_SIZE(e);
	k->bits.num.suffix = VAL_UNSIGNED | VAL_LONG;
	k->type = CONST_NUM;
}

const out_val *gen_expr_sizeof(expr *e, out_ctx *octx)
{
	return out_new_l(octx, e->tree_type, SIZEOF_SIZE(e));
}

const out_val *gen_expr_str_sizeof(expr *e, out_ctx *octx)
{
	if(e->expr){
		idt_printf("sizeof expr:\n");
		print_expr(e->expr);
	}else{
		idt_printf("sizeof %s\n", type_to_str(e->bits.size_of.of_type));
	}

	if(e->what_of == what_sizeof)
		idt_printf("size = %d\n", SIZEOF_SIZE(e));

	UNUSED_OCTX();
}

void mutate_expr_sizeof(expr *e)
{
	e->f_const_fold = const_expr_sizeof;
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

const out_val *gen_expr_style_sizeof(expr *e, out_ctx *octx)
{
	stylef("%s(", sizeof_what(e->what_of));

	if(e->expr)
		IGNORE_PRINTGEN(gen_expr(e->expr, octx));
	else
		stylef("%s", type_to_str(e->bits.size_of.of_type));

	stylef(")");

	UNUSED_OCTX();
}
