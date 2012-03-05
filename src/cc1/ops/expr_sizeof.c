#include "ops.h"

const char *str_expr_sizeof()
{
	return "sizeof";
}

void fold_expr_sizeof(expr *e, symtable *stab)
{
	if(!e->expr->expr_is_sizeof)
		fold_expr(e->expr, stab);

	e->tree_type = decl_new();
	e->tree_type->type->primitive = type_int;
}

void gen_expr_sizeof_1(expr *e)
{
	ICE("TODO: init with %s", e->f_str());
}

void gen_expr_sizeof(expr *e, symtable *stab)
{
	decl *d = e->expr->tree_type;
	(void)stab;

	asm_output_new(
		asm_out_type_push,
		asm_operand_new_val(decl_size(d)),
		NULL);

	asm_comment("sizeof %s%s", e->expr->expr_is_sizeof ? "type " : "", decl_to_str(d));

	/*asm_temp(1, "push %d ; sizeof %s%s", decl_size(d), e->expr->expr_is_sizeof ? "type " : "", decl_to_str(d));*/
}

void gen_expr_str_sizeof(expr *e, symtable *stab)
{
	(void)stab;
	if(e->expr->expr_is_sizeof){
		idt_printf("sizeof %s\n", decl_to_str(e->expr->tree_type));
	}else{
		idt_printf("sizeof expr:\n");
		print_expr(e->expr);
	}
}
