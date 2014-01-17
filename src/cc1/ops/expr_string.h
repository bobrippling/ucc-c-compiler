func_mutate_expr mutate_expr_str;
func_str     str_expr_str;

func_fold    fold_expr_str;

func_gen     gen_expr_str;

func_gen     gen_expr_str_str;
func_gen     gen_expr_style_str;

void expr_mutate_str(
		expr *e,
		char *s, size_t len,
		int wide,
		where *w, symtable *stab);
