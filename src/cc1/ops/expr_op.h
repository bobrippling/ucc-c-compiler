func_fold    fold_expr_op;
func_gen     gen_expr_op;
func_const   const_expr_op;
func_str     str_expr_op;
func_gen     gen_expr_str_op;
func_mutate_expr mutate_expr_op;
func_gen     gen_expr_style_op;

#define op_deref_expr(e) ((e)->lhs)

void op_get_asm(enum op_type op,
		const char **pinstruct,
		const char **plhs, const char **prhs,
		const char **ppre,
		const char **pret);
