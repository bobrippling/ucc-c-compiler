#ifndef GEN_ASM_H
#define GEN_ASM_H

void gen_asm(symtable *);
void walk_expr(expr *e, symtable *stab);

#endif
