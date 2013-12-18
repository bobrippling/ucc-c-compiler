#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global(decl *d);
void gen_asm_extern(decl *d);

void gen_asm(symtable_global *);
void gen_expr(expr *e);
void gen_unused_expr(expr *e); /* attempts a lea first */
void lea_expr(expr *e);
void gen_stmt(stmt *t);

#endif
