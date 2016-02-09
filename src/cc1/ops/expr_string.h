EXPR_DEFS(str);

void expr_mutate_str(
		expr *e,
		char *s, size_t len,
		int wide,
		where *w, symtable *stab);

expr *expr_new_str(char *, size_t, int wide, where *, symtable *stab);
