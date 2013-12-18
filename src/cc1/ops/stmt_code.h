STMT_DEFS(code);

void gen_block_decls(symtable *stab);
void fold_block_decls(symtable *stab, stmt **pinit_blk);

void gen_stmt_code_m1(stmt *s, int m1);
