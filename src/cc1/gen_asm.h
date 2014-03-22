#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global(decl *d, out_ctx *octx);
void gen_asm_extern(decl *d, out_ctx *octx);

void gen_asm(symtable_global *globs,
		const char *fname, const char *compdir);

out_val *gen_expr(expr *e, out_ctx *);
out_val *lea_expr(expr *e, out_ctx *);
void gen_stmt(stmt *t, out_ctx *);

#endif
