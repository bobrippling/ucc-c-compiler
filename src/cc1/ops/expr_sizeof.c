#include "../data_structs.h"
#include "expr_sizeof.h"

void fold_expr_sizeof(expr *e, symtable *stab)
{
	if(!e->expr->expr_is_sizeof)
		fold_expr(e->expr, stab);

	e->tree_type->type->primitive = type_int;
}

void gen_expr_sizeof(expr *e, symtable *stab)
{
	decl *d = e->expr->tree_type;
	asm_temp(1, "push %d ; sizeof %s%s", decl_size(d), e->expr->expr_is_sizeof ? "type " : "", decl_to_str(d));
}
