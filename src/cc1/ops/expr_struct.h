EXPR_DEFS(struct);

expr *expr_new_struct(expr *sub, int dot, expr *ident);
expr *expr_new_struct_mem(expr *sub, int dot, decl *);
