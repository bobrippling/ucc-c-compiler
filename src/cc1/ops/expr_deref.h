EXPR_DEFS(deref);

#define expr_deref_what(e) ((e)->expr)

expr *expr_new_deref(expr *);
