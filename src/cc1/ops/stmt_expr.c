#include <string.h>

#include "ops.h"
#include "stmt_expr.h"

#include "../out/basic_block/bb.h"

const char *str_stmt_expr()
{
	return "expr";
}

void fold_stmt_expr(stmt *s)
{
	FOLD_EXPR(s->expr, s->symtab);
	if(!s->freestanding && !s->expr->freestanding && !type_ref_is_void(s->expr->tree_type))
		cc1_warn_at(&s->expr->where, 0, WARN_UNUSED_EXPR,
				"unused expression (%s)", s->expr->f_str());
}

basic_blk *gen_stmt_expr(stmt *s, basic_blk *bb)
{
	unsigned pre_vcount = bb_vcount(bb);
	char *sp;

	bb = gen_expr(s->expr, bb);

	if((fopt_mode & FOPT_ENABLE_ASM) == 0
	|| !s->expr
	|| expr_kind(s->expr, funcall)
	|| !(sp = s->expr->bits.ident.spel)
	|| strcmp(sp, ASM_INLINE_FNAME))
	{
		if(s->expr_no_pop)
			pre_vcount++;
		else
			out_pop(bb); /* cancel the implicit push from gen_expr() above */

		out_comment(bb, "end of %s-stmt", s->f_str());

		UCC_ASSERT(bb_vcount(bb) == pre_vcount,
				"vcount changed over %s statement (%d -> %d)",
				s->expr->f_str(),
				bb_vcount(bb), pre_vcount);
	}

	return bb;
}

basic_blk *style_stmt_expr(stmt *s, basic_blk *bb)
{
	bb = gen_expr(s->expr, bb);
	stylef(";\n");
	return bb;
}

static int expr_passable(stmt *s)
{
	/*
	 * TODO: ({}) - return inside?
	 * if we have a funcall marked noreturn, we're not passable
	 */
	if(expr_kind(s->expr, funcall))
		return expr_func_passable(s->expr);

	if(expr_kind(s->expr, stmt))
		return fold_passable(s->expr->code);

	return 1;
}

void mutate_stmt_expr(stmt *s)
{
	s->f_passable = expr_passable;
}
