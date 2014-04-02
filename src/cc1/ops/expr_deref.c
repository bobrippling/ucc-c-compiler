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
		warn_at(&ptr->where, "dereference of noderef expression");

	/* check for *&x */
	if(expr_kind(ptr, addr) && !ptr->expr_addr_implicit)
		warn_at(&ptr->where, "possible optimisation for *& expression");

	fold_check_bounds(ptr, 0);

	e->tree_type = type_pointed_to(ptr->tree_type);
}

static out_val *gen_expr_deref_lea(expr *e, out_ctx *octx)
{
	/* a dereference */
	return gen_expr(expr_deref_what(e), octx); /* skip over the *() bit */
}

out_val *gen_expr_deref(expr *e, out_ctx *octx)
{
	return out_deref(
			octx,
			gen_expr_deref_lea(e, octx));
}

out_val *gen_expr_str_deref(expr *e, out_ctx *octx)
{
	idt_printf("deref, size: %s\n", type_to_str(e->tree_type));
	gen_str_indent++;
	print_expr(expr_deref_what(e));
	gen_str_indent--;
	UNUSED_OCTX();
}

static void const_expr_deref(expr *e, consty *k)
{
	expr *from = expr_deref_what(e);

	const_fold(from, k);

	switch(k->type){
		case CONST_STRK:
		{
			stringlit *sv = k->bits.str->lit;
			if(k->offset < 0 || (unsigned)k->offset > sv->len){
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
		case CONST_NUM:
		case CONST_ADDR:
			k->type = CONST_ADDR_OR_NEED_TREF(from->tree_type);
			/* *(int [10])a -> still need_addr */
		default:
			break;
	}
}

void mutate_expr_deref(expr *e)
{
	e->f_const_fold = const_expr_deref;

	/* unconditionally an lvalue */
	e->f_lea = gen_expr_deref_lea;
}

expr *expr_new_deref(expr *of)
{
	expr *e = expr_new_wrapper(deref);
	expr_deref_what(e) = of;
	return e;
}

out_val *gen_expr_style_deref(expr *e, out_ctx *octx)
{
	stylef("*(");
	IGNORE_PRINTGEN(gen_expr(expr_deref_what(e), octx));
	stylef(")");
	return NULL;
}
