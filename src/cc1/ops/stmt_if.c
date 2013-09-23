#include <stdlib.h>

#include "ops.h"
#include "stmt_if.h"
#include "stmt_for.h"
#include "../out/lbl.h"
#include "../out/basic_block.h"
#include "../fold_sym.h"

const char *str_stmt_if()
{
	return "if";
}

void flow_fold(stmt_flow *flow, symtable **pstab)
{
	if(flow){
		decl **i;

		*pstab = flow->for_init_symtab;

		/* sanity check on _flow_ vars only */
		for(i = (*pstab)->decls; i && *i; i++){
			decl *const d = *i;

			fold_decl(d, *pstab, &flow->init_blk);

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

basic_blk *flow_gen(stmt_flow *flow, symtable *stab, basic_blk *bb)
{
	gen_code_decls(stab);

	if(flow){
		gen_code_decls(flow->for_init_symtab);

		if(flow->init_blk)
			bb = gen_stmt(flow->init_blk, bb);
		/* also generates decls on the flow->inits statement */
	}

	return bb;
}

void fold_stmt_if(stmt *s)
{
	symtable *stab = s->symtab;

	flow_fold(s->flow, &stab);

	FOLD_EXPR(s->expr, stab);

	fold_check_expr(s->expr, FOLD_CHK_BOOL, s->f_str());

	fold_stmt(s->lhs);
	if(s->rhs)
		fold_stmt(s->rhs);
}

basic_blk *gen_stmt_if(stmt *s, basic_blk *bb)
{
	struct basic_blk *b_true, *b_false;
	struct basic_blk_phi *b_join;

	bb = gen_expr(s->expr, flow_gen(s->flow, s->symtab, bb));

	bb_split_new(bb, &b_true, &b_false, &b_join, "if");

	b_true = gen_stmt(s->lhs, b_true);
	bb_phi_incoming(b_join, b_true);

	if(s->rhs){
		b_false = gen_stmt(s->rhs, b_false);
		bb_phi_incoming(b_join, b_false);
	}

	return bb_phi_next(b_join);
}

basic_blk *style_stmt_if(stmt *s, basic_blk *bb)
{
	stylef("if(");
	bb = gen_expr(s->expr, bb);
	stylef(")\n");
	bb = gen_stmt(s->lhs, bb);

	if(s->rhs){
		stylef("else\n");
		bb = gen_stmt(s->rhs, bb);
	}

	return bb;
}

static int if_passable(stmt *s)
{
	return (s->rhs ? fold_passable(s->rhs) : 1) || fold_passable(s->lhs);
}

void mutate_stmt_if(stmt *s)
{
	s->f_passable = if_passable;
}
