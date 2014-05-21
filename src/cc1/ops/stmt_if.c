#include <stdlib.h>

#include "ops.h"
#include "stmt_if.h"
#include "stmt_for.h"
#include "../out/lbl.h"
#include "../fold_sym.h"
#include "../out/dbg.h"

const char *str_stmt_if()
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
		for(i = (*pstab)->decls; i && *i; i++){
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
		}

		if(flow->init_blk)
			fold_stmt(flow->init_blk);
	}
}

void flow_gen(
		stmt_flow *flow, symtable *stab,
		const char *endlbls[2], out_ctx *octx)
{
	gen_block_decls(stab, &endlbls[0], octx);
	endlbls[1] = NULL;

	if(flow){
		if(stab != flow->for_init_symtab)
			gen_block_decls(flow->for_init_symtab, &endlbls[1], octx);

		if(flow->init_blk)
			gen_stmt(flow->init_blk, octx);
		/* also generates decls on the flow->inits statement */
	}
}

void flow_end(const char *endlbls[2], out_ctx *octx)
{
	int i;
	for(i = 0; i < 2; i++)
		if(endlbls[i])
			out_dbg_label(octx, endlbls[i]);
}

void fold_stmt_if(stmt *s)
{
	fold_check_expr(s->expr, FOLD_CHK_BOOL, s->f_str());

	fold_stmt(s->lhs);
	if(s->rhs)
		fold_stmt(s->rhs);
}

void gen_stmt_if(stmt *s, out_ctx *octx)
{
	out_blk *blk_true = out_blk_new(octx, "if_true");
	out_blk *blk_false = out_blk_new(octx, "if_false");
	out_blk *blk_fi = out_blk_new(octx, "fi");
	const char *el[2];
	const out_val *cond;

	flow_gen(s->flow, s->symtab, el, octx);
	cond = gen_expr(s->expr, octx);

	out_ctrl_branch(octx, cond, blk_true, blk_false);

	out_current_blk(octx, blk_true);
	{
		gen_stmt(s->lhs, octx);
		out_ctrl_transfer(octx, blk_fi, NULL);
	}

	out_current_blk(octx, blk_false);
	{
		if(s->rhs)
			gen_stmt(s->rhs, octx);
		out_ctrl_transfer(octx, blk_fi, NULL);
	}

	out_current_blk(octx, blk_fi);
	flow_end(el, octx);
}

void style_stmt_if(stmt *s, out_ctx *octx)
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

static int if_passable(stmt *s)
{
	return (s->rhs ? fold_passable(s->rhs) : 1) || fold_passable(s->lhs);
}

void init_stmt_if(stmt *s)
{
	s->f_passable = if_passable;
}
