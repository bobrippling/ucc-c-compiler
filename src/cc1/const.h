#ifndef CONST_H
#define CONST_H

void const_fold(expr *e, consty *);
/*int const_expr_is_const(expr *e);*/
int const_expr_and_zero(expr *e);
int const_expr_and_non_zero(expr *e);
/*long const_expr_value(expr *e);*/
void const_fold_intval(expr *e, intval *);
intval_t const_fold_val(expr *e);

#endif
