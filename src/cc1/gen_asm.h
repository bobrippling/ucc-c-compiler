#ifndef GEN_ASM_H
#define GEN_ASM_H

extern char *curfunc_lblfin;

void gen_asm_global_w_store(decl *d, int emit_tenatives, out_ctx *octx);
void gen_asm_extern(decl *d, out_ctx *octx);

void gen_set_sym_outval(out_ctx *octx, sym *sym, const out_val *v);

void gen_vla_arg_sideeffects(decl *d, out_ctx *octx);

#ifdef DBG_H
void gen_asm(
		symtable_global *globs,
		const char *fname, const char *compdir,
		struct out_dbg_filelist **pfilelist,
		const char *producer);
#endif

#include "parse_fold_error.h"

/* easy-to-search-for macro for non-const use inside the gen functions */
#define GEN_CONST_CAST(T, expr) ((T)(e))

const out_val *gen_expr(const expr *e, out_ctx *) ucc_wur;
void gen_stmt(const struct stmt *t, out_ctx *);
#define gen_func_stmt gen_stmt

ucc_wur
const out_val *gen_call(
		expr *maybe_exp, decl *maybe_dfn,
		const out_val *fnval,
		const out_val **args, out_ctx *octx,
		const where *loc);

ucc_wur
const out_val *gen_decl_addr(out_ctx *, decl *);

void gen_asm_emit_type(out_ctx *, type *);

/* temporary until the f_gen() logic from expr is pulled out
 * into asm, print and style backends */
void IGNORE_PRINTGEN(const out_val *v);

#endif
