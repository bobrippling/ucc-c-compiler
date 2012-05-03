func_fold    fold_expr_op;
func_gen     gen_expr_op;
func_const   const_expr_op;
func_str     str_expr_op;
func_gen     gen_expr_str_op;
func_mutate_expr mutate_expr_op;
func_gen     gen_expr_style_op;

#define op_deref_expr(e) ((e)->lhs)
