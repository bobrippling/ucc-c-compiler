func_fold    fold_expr_cast;
func_gen     gen_expr_cast;
func_gen_1   gen_expr_1_cast;
func_const   const_expr_cast;
func_str     str_expr_cast;
func_gen     gen_expr_str_cast;
func_mutate_expr mutate_expr_cast;
func_gen     gen_expr_style_cast;

void fold_expr_cast_descend(expr *e, symtable *stab, int descend);
