EXPR_DEFS(block);

void expr_block_set_ty(decl *db, type *retty, symtable *scope);
void expr_block_got_params(expr *e, symtable *symtab, struct funcargs *args);
void expr_block_got_code(expr *e, struct stmt *code);

expr *expr_new_block(type *rt, struct funcargs *args);
