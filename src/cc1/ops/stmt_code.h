STMT_DEFS(code);

void gen_block_decls(symtable *stab, const char **dbg_end_lbl, out_ctx *);
void fold_shadow_dup_check_block_decls(symtable *stab);

void gen_stmt_code_m1(stmt *s, int m1, out_ctx *);
