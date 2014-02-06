STMT_DEFS(code);

void gen_block_decls(symtable *stab, const char **dbg_end_lbl);
void fold_block_decls(symtable *stab, stmt **pinit_blk);
void stmt_code_got_decls(stmt *code);
