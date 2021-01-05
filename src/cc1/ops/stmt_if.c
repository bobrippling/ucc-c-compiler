#include <stdlib.h>
#include <assert.h>

#include "ops.h"
#include "stmt_if.h"
#include "stmt_for.h"
#include "../out/lbl.h"
#include "../fold_sym.h"
#include "../out/dbg.h"
#include "../out/dbg_lbl.h"

const char *str_stmt_if(void)
{
	return "if";
}

void flow_fold(stmt_flow *flow, symtable **pstab)
{
	if(flow){
		decl **i;

		*pstab = flow->for_init_symtab;

		fold_shadow_dup_check_block_decls(*pstab);

		/* sanity check on _flow_ vars only */
		for(i = symtab_decls(*pstab); i && *i; i++){
			decl *const d = *i;

			switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
				case store_auto:
				case store_default:
				case store_register:
					break;
				default:
					die_at(&d->where, "%s variable in statement-initialisation",
							decl_store_to_str(d->store));
			}

			/* block decls/for-init decls must be complete */
			fold_check_decl_complete(d);

			if(d->bits.var.init.expr)
				FOLD_EXPR(d->bits.var.init.expr, *pstab);
		}
	}
}

void flow_gen(
		stmt_flow *flow,
		symtable *stab,
		struct out_dbg_lbl *pushed_lbls[2][2],
		out_ctx *octx)
{
	gen_block_decls(stab, pushed_lbls[0], octx);

	if(flow && stab != flow->for_init_symtab)
		gen_block_decls(flow->for_init_symtab, pushed_lbls[1], octx);
}

void flow_end(
		stmt_flow *flow,
		symtable *stab,
		struct out_dbg_lbl *pushed_lbls[2][2],
		out_ctx *octx)
{
	/* generate the braced scope first, then the for-control-variable's */
	gen_scope_leave_parent(stab, octx);
	gen_block_decls_dealloca(stab, pushed_lbls[0], octx);

	if(flow && stab != flow->for_init_symtab){
		assert(stab->parent == flow->for_init_symtab);

		gen_scope_leave_parent(flow->for_init_symtab, octx);

		gen_block_decls_dealloca(flow->for_init_symtab, pushed_lbls[1], octx);
	}
}

void fold_stmt_if(stmt *s)
{
	(void)!fold_check_expr(s->expr, FOLD_CHK_BOOL, s->f_str());

	fold_stmt(s->lhs);
	if(s->rhs)
		fold_stmt(s->rhs);
}

void gen_stmt_if(const stmt *s, out_ctx *octx)
{
	out_blk *blk_true = out_blk_new(octx, "if_true");
	out_blk *blk_false = out_blk_new(octx, "if_false");
	out_blk *blk_fi = out_blk_new(octx, "fi");
	struct out_dbg_lbl *el[2][2];
	const out_val *cond;

	flow_gen(s->flow, s->symtab, el, octx);
	cond = gen_expr(s->expr, octx);

	out_ctrl_branch(octx, cond, blk_true, blk_false);

	out_current_blk(octx, blk_true);
	{
		gen_stmt(s->lhs, octx);
		out_ctrl_transfer(octx, blk_fi, NULL, NULL, 0);
	}

	out_current_blk(octx, blk_false);
	{
		if(s->rhs)
			gen_stmt(s->rhs, octx);
		out_ctrl_transfer(octx, blk_fi, NULL, NULL, 0);
	}

	out_current_blk(octx, blk_fi);
	flow_end(s->flow, s->symtab, el, octx);
}

void dump_stmt_if(const stmt *s, dump *ctx)
{
	dump_desc_stmt(ctx, "if", s);

	dump_inc(ctx);

	dump_flow(s->flow, ctx);

	dump_expr(s->expr, ctx);
	dump_stmt(s->lhs, ctx);
	if(s->rhs)
		dump_stmt(s->rhs, ctx);
	dump_dec(ctx);
}

void style_stmt_if(const stmt *s, out_ctx *octx)
{
	stylef("if(");
	IGNORE_PRINTGEN(gen_expr(s->expr, octx));
	stylef(")\n");
	gen_stmt(s->lhs, octx);

	if(s->rhs){
		stylef("else\n");
		gen_stmt(s->rhs, octx);
	}
}

static int if_passable(stmt *s, int break_means_passable)
{
	return (s->rhs ? fold_passable(s->rhs, break_means_passable) : 1) || fold_passable(s->lhs, break_means_passable);
}

void init_stmt_if(stmt *s)
{
	s->f_passable = if_passable;
}
