#include "ops.h"

const char *expr_str_sizeof()
{
	return "sizeof";
}

void expr_fold_sizeof(expr *e, symtable *stab)
{
	if(!e->expr->expr_is_sizeof)
		fold_expr(e->expr, stab);

	e->tree_type = decl_new();
	e->tree_type->type->primitive = type_int;
}

void expr_gen_sizeof_1(expr *e)
{
	ICE("TODO: init with %s", e->f_str());
}

void expr_gen_sizeof(expr *e, symtable *stab)
{
	decl *d = e->expr->tree_type;
	(void)stab;

	asm_temp(1, "push %d ; sizeof %s%s", decl_size(d), e->expr->expr_is_sizeof ? "type " : "", decl_to_str(d));
}

void expr_gen_str_sizeof(expr *e)
{
	if(e->expr->expr_is_sizeof){
		idt_printf("sizeof %s\n", decl_to_str(e->expr->tree_type));
	}else{
		idt_printf("sizeof expr:\n");
		print_expr(e->expr);
	}
}
