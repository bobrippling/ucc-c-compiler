#ifndef EXPR_ASSIGN_H
#define EXPR_ASSIGN_H

func_fold    fold_expr_assign;
func_gen     gen_expr_assign;
func_str     str_expr_assign;
func_gen     gen_expr_str_assign;
func_mutate_expr mutate_expr_assign;
func_gen     gen_expr_style_assign;

int expr_is_lvalue(expr *e);

#endif
