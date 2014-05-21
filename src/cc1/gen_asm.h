#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global(decl *d, out_ctx *octx);
void gen_asm_extern(decl *d, out_ctx *octx);

void gen_asm(symtable_global *globs,
		const char *fname, const char *compdir);

const out_val *gen_expr(expr *e, out_ctx *) ucc_wur;
const out_val *lea_expr(expr *e, out_ctx *) ucc_wur;
void gen_stmt(stmt *t, out_ctx *);

/* temporary until the f_gen() logic from expr is pulled out
 * into asm, print and style backends */
void IGNORE_PRINTGEN(const out_val *v);

#endif
