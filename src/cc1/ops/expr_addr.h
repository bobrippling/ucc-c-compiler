EXPR_DEFS(addr);

/* differs from lvalue - allows arrays and functions */
int expr_is_addressable(expr *e);

expr *expr_addr_target(const expr *);

expr *expr_new_addr_lbl(char *lbl, int static_ctx);
expr *expr_new_addr(expr *);
