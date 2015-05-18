EXPR_DEFS(cast);

#define expr_cast_child(e) ((e)->expr)

void fold_expr_cast_descend(
		expr *e, symtable *stab, int descend);
