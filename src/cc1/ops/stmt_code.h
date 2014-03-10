STMT_DEFS(code);

void gen_block_decls(symtable *stab, const char **dbg_end_lbl);
void gen_block_decls_end(symtable *stab, const char *endlbl);

void fold_shadow_dup_check_block_decls(symtable *stab);
