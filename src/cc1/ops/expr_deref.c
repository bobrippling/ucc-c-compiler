#include <string.h>

#include "ops.h"
#include "expr_deref.h"
#include "../type_nav.h"
#include "../type_is.h"

const char *str_expr_deref()
{
	return "dereference";
}

void fold_expr_deref(expr *e, symtable *stab)
{
	expr *ptr;

	ptr = FOLD_EXPR(expr_deref_what(e), stab);

	if(!type_is_ptr(ptr->tree_type)){
		fold_had_error = 1;
		warn_at_print_error(&ptr->where,
				"indirection applied to '%s'",
				type_to_str(ptr->tree_type));
		e->tree_type = ptr->tree_type;
		return;
	}

	if(expr_attr_present(ptr, attr_noderef))
		cc1_warn_at(&ptr->where, attr_noderef,
				"dereference of noderef expression");

	fold_check_bounds(ptr, 0);

	e->tree_type = type_is_ptr(ptr->tree_type);
}

const out_val *gen_expr_deref(const expr *e, out_ctx *octx)
{
	/* lea - we're an lvalue */
	return gen_expr(expr_deref_what(e), octx);
}

void dump_expr_deref(const expr *e, dump *ctx)
{
	dump_desc_expr(ctx, "dereference", e);
	dump_inc(ctx);
	dump_expr(expr_deref_what(e), ctx);
	dump_dec(ctx);
}

static void const_expr_deref(expr *e, consty *k)
{
	expr *from = expr_deref_what(e);

	const_fold(from, k);

	switch(k->type){
		case CONST_STRK:
		{
			stringlit *sv = k->bits.str->lit;

			/* check type we're supposed to be dereferencing as,
			 * should be char *
			 */
			if(!type_is_primitive_anysign(type_is_ptr(from->tree_type), type_nchar)){
				k->type = CONST_NO;
				break;
			}

			if(k->offset < 0 || (unsigned)k->offset >= sv->len){
				k->type = CONST_NO;
			}else{
				long off = k->offset;

				UCC_ASSERT(!sv->wide, "TODO: constant wchar_t[] deref");

				CONST_FOLD_LEAF(k);
				k->type = CONST_NUM;
				k->bits.num.val.i = sv->str[off];
			}
			break;
		}
		case CONST_NEED_ADDR:
			k->type = CONST_NO;
			break;

		case CONST_NUM:
		{
			integral_t num = k->bits.num.val.i;
			CONST_FOLD_LEAF(k);
			k->bits.addr.is_lbl = 0;
			k->bits.addr.bits.memaddr = num;
		} /* fall */
		case CONST_ADDR:
			/* *(int [10])a -> still need_addr */
			k->type = CONST_ADDR_OR_NEED_TYPE(e->tree_type);
			break;
		case CONST_NO:
			break;
	}
}

void mutate_expr_deref(expr *e)
{
	e->f_const_fold = const_expr_deref;

	e->f_islval = expr_is_lval_always;
}

expr *expr_new_deref(expr *of)
{
	expr *e = expr_new_wrapper(deref);
	expr_deref_what(e) = of;
	return e;
}

const out_val *gen_expr_style_deref(const expr *e, out_ctx *octx)
{
	stylef("*(");
	IGNORE_PRINTGEN(gen_expr(expr_deref_what(e), octx));
	stylef(")");
	return NULL;
}
