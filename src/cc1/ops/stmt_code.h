STMT_DEFS(code);

#include "../../util/compiler.h"

struct out_dbg_lbl;


/* folding */
void fold_check_scope_entry(where *, const char *desc,
		symtable *const s_from, symtable *const s_to);

void fold_shadow_dup_check_block_decls(symtable *stab);


/* native code-gen */
void gen_block_decls(
		symtable *,
		struct out_dbg_lbl *[ucc_static_param 2],
		out_ctx *);

void gen_block_decls_dealloca(
		symtable *stab,
		struct out_dbg_lbl *pushed_lbls[ucc_static_param 2],
		out_ctx *octx);

void gen_scope_leave(symtable *s_from, symtable *s_to, out_ctx *octx);
void gen_scope_leave_parent(symtable *s_from, out_ctx *octx);

void gen_stmt_code_m1(
		const stmt *s,
		int m1,
		struct out_dbg_lbl *pushed_lbls[ucc_static_param 2],
		out_ctx *octx);

void gen_stmt_code_m1_finish(
		const stmt *s,
		struct out_dbg_lbl *pushed_lbls[ucc_static_param 2],
		out_ctx *octx);


/* ir code-gen */
void gen_ir_block_decls(symtable *symtab, struct irctx *ctx);

void gen_ir_stmt_code_m1(const stmt *s, struct irctx *ctx, const int m1);
