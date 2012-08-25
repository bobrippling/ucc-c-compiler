func_mutate_expr mutate_expr_deref;
func_str         str_expr_deref;

func_fold        fold_expr_deref;

func_gen         gen_expr_deref;
func_const       const_expr_deref;

func_gen         gen_expr_str_deref;
func_gen         gen_expr_style_deref;

#define expr_deref_what(e) ((e)->expr)
