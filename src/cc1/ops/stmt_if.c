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

void flow_gen(stmt_flow *flow, symtable *stab)
{
	gen_code_decls(stab);

	if(flow){
		gen_code_decls(flow->for_init_symtab);

		if(flow->init_blk)
			gen_stmt(flow->init_blk);
		/* also generates decls on the flow->inits statement */
	}
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

struct basic_blk *gen_stmt_if(stmt *s, struct basic_blk *b_from)
{
	struct basic_blk *b_true, *b_false, *b_join;

	b_from = gen_expr(s->expr,
			flow_gen(s->flow, s->symtab, b_from));

	bb_split(b_from, &b_true, &b_false);

	b_true = gen_stmt(s->lhs, b_true);

	if(s->rhs)
		b_false = gen_stmt(s->rhs, b_false);

	return bb_merge(b_true, b_false);
}

void style_stmt_if(stmt *s)
{
	stylef("if(");
	gen_expr(s->expr);
	stylef(")\n");
	gen_stmt(s->lhs);

	if(s->rhs){
		stylef("else\n");
		gen_stmt(s->rhs);
	}
}

static int if_passable(stmt *s)
{
	return (s->rhs ? fold_passable(s->rhs) : 1) || fold_passable(s->lhs);
}

void mutate_stmt_if(stmt *s)
{
	s->f_passable = if_passable;
}
