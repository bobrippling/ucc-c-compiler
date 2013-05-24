#include <stdlib.h>

#include "ops.h"
#include "stmt_if.h"
#include "stmt_for.h"
#include "../out/lbl.h"

const char *str_stmt_if()
{
	return "if";
}

void flow_fold(stmt_flow *flow, symtable **pstab)
{
	if(flow){
		decl **i;

		*pstab = flow->for_init_symtab;
		fold_symtab_scope(*pstab, &flow->inits);
		if(flow->inits)
			fold_stmt(flow->inits);

		/* sanity check on _flow_ vars only */
		for(i = (*pstab)->decls; i && *i; i++){
			decl *const d = *i;

			switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
				case store_auto:
				case store_default:
				case store_register:
					break;
				default:
					DIE_AT(&d->where, "%s variable in statement-initialisation",
							decl_store_to_str(d->store));
			}
		}
	}
}

void flow_gen(stmt_flow *flow, symtable *stab)
{
	gen_code_decls(stab);

	if(flow){
		gen_code_decls(flow->for_init_symtab);

		if(flow->inits)
			gen_stmt(flow->inits);
		/* also generates decls on the flow->inits statement */
	}
}

void fold_stmt_if(stmt *s)
{
	symtable *stab = s->symtab;

	flow_fold(s->flow, &stab);

	FOLD_EXPR(s->expr, stab);

	fold_need_expr(s->expr, s->f_str(), 1);

	fold_stmt(s->lhs);
	if(s->rhs)
		fold_stmt(s->rhs);
}

void gen_stmt_if(stmt *s)
{
	char *lbl_else = out_label_code("else");
	char *lbl_fi   = out_label_code("fi");

	flow_gen(s->flow, s->symtab);
	gen_expr(s->expr, s->symtab);

	out_jfalse(lbl_else);

	gen_stmt(s->lhs);
	out_push_lbl(lbl_fi, 0);
	out_jmp();

	out_label(lbl_else);
	if(s->rhs)
		gen_stmt(s->rhs);
	out_label(lbl_fi);

	free(lbl_else);
	free(lbl_fi);
}

void style_stmt_if(stmt *s)
{
	stylef("if( [expr] )");
	gen_stmt(s->lhs);
	if(s->rhs){
		stylef("else");
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
