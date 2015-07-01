EXPR_DEFS(cast);

#define expr_cast_child(e) ((e)->expr)
#define expr_cast_is_lval2rval(e) (!(e)->bits.cast_to)

void fold_expr_cast_descend(
		expr *e, symtable *stab, int descend);
