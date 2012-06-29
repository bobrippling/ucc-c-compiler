func_fold_stmt fold_stmt_for;
func_gen_stmt  gen_stmt_for;
func_str_stmt  str_stmt_for;

expr *fold_for_if_init_decls(stmt *s);
func_mutate_stmt mutate_stmt_for;

int fold_code_escapable(stmt *s);
