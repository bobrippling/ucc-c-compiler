EXPR_DEFS(compound_lit);

expr *expr_new_compound_lit(
		type *t, struct decl_init *init,
		int static_ctx);
