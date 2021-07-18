EXPR_DEFS(str);

void expr_mutate_str(
		expr *e,
		struct cstring *,
		where *w, symtable *stab);

expr *expr_new_str(struct cstring *, where *, symtable *stab);
