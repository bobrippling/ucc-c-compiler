EXPR_DEFS(comma);

expr *expr_new_comma2(expr *lhs, expr *rhs, int compiler_gen);
#define expr_new_comma() expr_new_wrapper(comma)
