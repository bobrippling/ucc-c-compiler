#include "ops.h"
#include "expr_deref.h"

const char *str_expr_deref()
{
	return "deref";
}

void fold_expr_deref(expr *e, symtable *stab)
{
	fold_expr(expr_deref_what(e), stab);

	if(decl_attr_present(expr_deref_what(e)->tree_type->attr, attr_noderef))
		WARN_AT(&expr_deref_what(e)->where, "dereference of noderef expression");

	/* check for *&x */
	if(expr_kind(expr_deref_what(e), addr))
		WARN_AT(&expr_deref_what(e)->where, "possible optimisation for *& expression");

	e->tree_type = decl_ptr_depth_dec(decl_copy(expr_deref_what(e)->tree_type), &e->where);
}

void gen_expr_deref_store(expr *e, symtable *stab)
{
	/* a dereference */
	gen_expr(expr_deref_what(e), stab); /* skip over the *() bit */
	out_comment("pointer on stack");
}

void gen_expr_deref(expr *e, symtable *stab)
{
	gen_expr(expr_deref_what(e), stab);
	out_deref();
}

void gen_expr_str_deref(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("deref, size: %s\n", decl_to_str(e->tree_type));
	gen_str_indent++;
	print_expr(expr_deref_what(e));
	gen_str_indent--;
}

void mutate_expr_deref(expr *e)
{
	e->f_store = gen_expr_deref_store;
}

expr *expr_new_deref(expr *of)
{
	expr *e = expr_new_wrapper(deref);
	expr_deref_what(e) = of;
	return e;
}

void gen_expr_style_deref(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
