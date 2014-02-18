func_fold        fold_expr_block;
func_gen         gen_expr_block;
func_str         str_expr_block;

func_mutate_expr mutate_expr_block;

func_gen         gen_expr_str_block;
func_gen         gen_expr_style_block;

void expr_block_set_ty(decl *db, type *retty, symtable *scope);
void expr_block_got_params(symtable *symtab, funcargs *args);
