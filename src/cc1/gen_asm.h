#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global(decl *d);
void gen_asm_extern(decl *d);

void gen_asm(symtable_global *);

struct basic_blk;
struct basic_blk *gen_expr(expr *e, struct basic_blk *);
struct basic_blk *lea_expr(expr *e, struct basic_blk *);
struct basic_blk *gen_stmt(stmt *t, struct basic_blk *);

#endif
