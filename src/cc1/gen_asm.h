#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global(decl *d);

void gen_asm(symtable *);
void gen_expr(expr *e, symtable *stab);
void gen_stat(stat *t);

#endif
