#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global_w_store(decl *d, int emit_tenatives, out_ctx *octx);
void gen_asm_extern(decl *d, out_ctx *octx);

void gen_set_sym_outval(out_ctx *octx, sym *sym, const out_val *v);

#ifdef DBG_H
void gen_asm(
		symtable_global *globs,
		const char *fname, const char *compdir,
		struct out_dbg_filelist **pfilelist);
#endif

const out_val *gen_expr(expr *e, out_ctx *) ucc_wur;
void gen_stmt(struct stmt *t, out_ctx *);

/* temporary until the f_gen() logic from expr is pulled out
 * into asm, print and style backends */
void IGNORE_PRINTGEN(const out_val *v);

#endif
