EXPR_DEFS(str);

void expr_mutate_str(
		expr *e,
		char *s, size_t len,
		int wide,
		where *w, symtable *stab);
