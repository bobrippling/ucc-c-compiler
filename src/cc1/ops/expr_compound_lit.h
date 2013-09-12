func_fold        fold_expr_compound_lit;
func_gen         gen_expr_compound_lit;
func_str         str_expr_compound_lit;
func_gen         gen_expr_str_compound_lit;
func_mutate_expr mutate_expr_compound_lit;
func_gen         gen_expr_style_compound_lit;

expr *expr_new_compound_lit(type_ref *t, decl_init *init);
