#include "ops.h"
#include "expr_deref.h"

const char *str_expr_deref()
{
	return "deref";
}

void fold_expr_deref(expr *e, symtable *stab)
{
	expr *ptr;

	ptr = FOLD_EXPR(expr_deref_what(e), stab);

	if(type_attr_present(ptr->tree_type, attr_noderef))
		WARN_AT(&ptr->where, "dereference of noderef expression");

	/* check for *&x */
	if(expr_kind(ptr, addr) && !ptr->expr_addr_implicit)
		WARN_AT(&ptr->where, "possible optimisation for *& expression");

	e->tree_type = type_ref_ptr_depth_dec(ptr->tree_type, &e->where);
}

void gen_expr_deref_lea(expr *e, symtable *stab)
{
	/* a dereference */
	gen_expr(expr_deref_what(e), stab); /* skip over the *() bit */
}

void gen_expr_deref(expr *e, symtable *stab)
{
	gen_expr_deref_lea(e, stab);
	out_deref();
}

void gen_expr_str_deref(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("deref, size: %s\n", type_ref_to_str(e->tree_type));
	gen_str_indent++;
	print_expr(expr_deref_what(e));
	gen_str_indent--;
}

void const_expr_deref(expr *e, consty *k)
{
	expr *from = expr_deref_what(e);

	const_fold(from, k);

	switch(k->type){
		case CONST_VAL:
		case CONST_ADDR:
		case CONST_STRK:
			k->type = CONST_ADDR_OR_NEED_TREF(from->tree_type);
			/* *(int [10])a -> still need_addr */
		default:
			break;
	}
}

void mutate_expr_deref(expr *e)
{
	e->f_lea = gen_expr_deref_lea;
	e->f_const_fold = const_expr_deref;
}

expr *expr_new_deref(expr *of)
{
	expr *e = expr_new_wrapper(deref);
	expr_deref_what(e) = of;
	return e;
}

void gen_expr_style_deref(expr *e, symtable *stab)
{
	stylef("*(");
	gen_expr(expr_deref_what(e), stab);
	stylef(")");
}
