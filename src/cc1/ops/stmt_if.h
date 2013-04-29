func_fold_stmt fold_stmt_if;
func_gen_stmt  gen_stmt_if;
func_str_stmt  str_stmt_if;

func_mutate_stmt mutate_stmt_if;

void flow_fold(stmt_flow *flow, symtable **pstab);
void flow_gen(stmt_flow *flow);
