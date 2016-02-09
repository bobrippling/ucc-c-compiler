EXPR_DEFS(sizeof);

type *expr_sizeof_type(expr *);

expr *expr_new_sizeof_type(type *, enum what_of what_of);
expr *expr_new_sizeof_expr(expr *, enum what_of what_of);
