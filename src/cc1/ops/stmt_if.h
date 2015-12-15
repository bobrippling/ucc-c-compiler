#include "../../util/compiler.h"

STMT_DEFS(if);

struct out_dbg_lbl;

void flow_fold(stmt_flow *flow, symtable **pstab);

void flow_gen(
		stmt_flow *flow,
		symtable *stab,
		struct out_dbg_lbl *pushed_lbls[ucc_static_param 2][2],
		out_ctx *octx);

void flow_end(
		stmt_flow *flow,
		symtable *stab,
		struct out_dbg_lbl *pushed_lbls[ucc_static_param 2][2],
		out_ctx *octx);

void flow_ir_gen(stmt_flow *flow, symtable *stab, struct irctx *ctx);
void flow_ir_end(stmt_flow *flow, symtable *stab, struct irctx *ctx);
