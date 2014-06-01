#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global_w_store(decl *d, int emit_tenatives);

void gen_asm(symtable_global *globs,
		const char *fname, const char *compdir);

void gen_expr(expr *e);
void lea_expr(expr *e);
void gen_stmt(stmt *t);

#endif
