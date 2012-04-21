#include "ops.h"

#define SIZEOF_WHAT(e) ((e)->expr ? (e)->expr->tree_type : (e)->decl)

const char *str_expr_sizeof()
{
	return "sizeof";
}

void fold_expr_sizeof(expr *e, symtable *stab)
{
	if(e->expr)
		fold_expr(e->expr, stab);

	e->tree_type = decl_new();
	/* size_t */
	e->tree_type->type->primitive = type_int;
	e->tree_type->type->is_signed = 0;
}

void gen_expr_sizeof_1(expr *e)
{
	ICE("TODO: init with %s", e->f_str());
}

void gen_expr_sizeof(expr *e, symtable *stab)
{
	decl *d = SIZEOF_WHAT(e);
	(void)stab;

	asm_temp(1, "push %d ; sizeof %s%s",
			decl_size(d),
			e->expr ? "" : "type ",
			decl_to_str(d));
}

void gen_expr_str_sizeof(expr *e, symtable *stab)
{
	(void)stab;
	if(e->expr){
		idt_printf("sizeof expr:\n");
		print_expr(e->expr);
	}else{
		idt_printf("sizeof %s\n", decl_to_str(e->decl));
	}
}

void mutate_expr_sizeof(expr *e)
{
	(void)e;
}

expr *expr_new_sizeof_decl(decl *d)
{
	expr *e = expr_new_wrapper(sizeof);
	e->decl = d;
	return e;
}

expr *expr_new_sizeof_expr(expr *sizeof_this)
{
	expr *e = expr_new_wrapper(sizeof);
	e->expr = sizeof_this;
	return e;
}

void gen_expr_style_sizeof(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
