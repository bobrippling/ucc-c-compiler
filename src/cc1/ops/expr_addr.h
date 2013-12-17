func_fold    fold_expr_addr;
func_gen     gen_expr_addr;
func_str     str_expr_addr;
func_gen     gen_expr_str_addr;
func_mutate_expr mutate_expr_addr;
func_gen     gen_expr_style_addr;

/* differs from lvalue - allows arrays and functions */
int expr_is_addressable(expr *e);
