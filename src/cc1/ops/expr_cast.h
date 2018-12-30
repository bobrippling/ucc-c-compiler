EXPR_DEFS(cast);

#define expr_cast_child(e) ((e)->expr)
#define expr_cast_is_lval2rval(e) (!(e)->bits.cast_to)
#define expr_cast_is_implicit(e) ((e)->expr_cast_implicit)

void fold_expr_cast_descend(
		expr *e, symtable *stab, int descend);

expr *expr_new_cast(expr *, type *cast_to, int implicit);
expr *expr_new_cast_lval_decay(expr *);
