func_mutate_expr mutate_expr_struct;
func_str         str_expr_struct;

func_fold        fold_expr_struct;

func_gen         gen_expr_struct;

func_gen         gen_expr_str_struct;
func_gen         gen_expr_style_struct;

#define struct_offset(rhs) rhs->tree_type->struct_offset
