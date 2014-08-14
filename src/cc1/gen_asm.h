#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global_w_store(decl *d, int emit_tenatives, out_ctx *octx);
void gen_asm_extern(decl *d, out_ctx *octx);

#ifdef DBG_H
void gen_asm(
		symtable_global *globs,
		const char *fname, const char *compdir,
		struct out_dbg_filelist **pfilelist);
#endif

extern int gen_had_error;

const out_val *gen_expr(const expr *e, out_ctx *) ucc_wur;
const out_val *lea_expr(const expr *e, out_ctx *) ucc_wur;
void gen_stmt(const struct stmt *t, out_ctx *);

/* temporary until the f_gen() logic from expr is pulled out
 * into asm, print and style backends */
void IGNORE_PRINTGEN(const out_val *v);

#endif
