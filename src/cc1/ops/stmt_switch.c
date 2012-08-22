#include <stdlib.h>

#include "ops.h"
#include "stmt_switch.h"
#include "../sue.h"
#include "../../util/alloc.h"
#include "../out/lbl.h"

const char *str_stmt_switch()
{
	return "switch";
}

void fold_switch_enum(stmt *sw, type *enum_type)
{
	const int nents = enum_nentries(enum_type->sue);
	stmt **titer;
	char *marks = umalloc(nents * sizeof *marks);
	int midx;

	/* for each case/default/case_range... */
	for(titer = sw->codes; titer && *titer; titer++){
		stmt *cse = *titer;
		int v, w;
		intval iv;

		if(cse->expr->expr_is_default)
			goto ret;

		const_fold_need_val(cse->expr, &iv);
		v = iv.val;

		if(stmt_kind(cse, case_range)){
			const_fold_need_val(cse->expr2, &iv);
			w = iv.val;
		}else{
			w = v;
		}

		for(; v <= w; v++){
			sue_member **mi;
			for(midx = 0, mi = enum_type->sue->members; *mi; midx++, mi++){
				enum_member *m = (*mi)->enum_member;

				const_fold_need_val(m->val, &iv);

				if(v == iv.val)
					marks[midx]++;
			}
		}
	}

	for(midx = 0; midx < nents; midx++)
		if(!marks[midx])
			cc1_warn_at(&sw->where, 0, 1, WARN_SWITCH_ENUM, "enum %s::%s not handled in switch",
					enum_type->sue->anon ? "" : enum_type->sue->spel,
					enum_type->sue->members[midx]->enum_member->spel);

ret:
	free(marks);
}

void fold_stmt_switch(stmt *s)
{
	type *typ;
	symtable *test_symtab = fold_stmt_test_init_expr(s, "switch");

	s->lbl_break = out_label_flow("switch");

	fold_expr(s->expr, test_symtab);

	fold_need_expr(s->expr, "switch", 0);

	OPT_CHECK(s->expr, "constant expression in switch");

	fold_stmt(s->lhs);
	/* FIXME: check for duplicate case values and at most, 1 default */

	/* check for an enum */
	typ = s->expr->tree_type->type;
	if(typ->primitive == type_enum){
		UCC_ASSERT(typ->sue, "no enum for enum type");
		fold_switch_enum(s, typ);
	}
}

void gen_stmt_switch(stmt *s)
{
	stmt **titer, *tdefault;

	tdefault = NULL;

	gen_expr(s->expr, s->symtab);

	out_comment("switch on this");

	for(titer = s->codes; titer && *titer; titer++){
		stmt *cse = *titer;
		intval iv;

		if(cse->expr->expr_is_default){
			tdefault = cse;
			continue;
		}

		const_fold_need_val(cse->expr, &iv);

		UCC_ASSERT(cse->expr->expr_is_default || !(iv.suffix & VAL_UNSIGNED),
				"don't handle unsigned yet");

		if(stmt_kind(cse, case_range)){
			char *skip = out_label_code("range_skip");
			intval max;

			/* TODO: proper signed/unsiged format - out_op() */
			const_fold_need_val(cse->expr2, &max);

			out_dup();
			out_push_iv(cse->expr->tree_type, &iv);

			out_op(op_lt);
			out_jtrue(skip);

			out_dup();
			out_push_iv(cse->expr2->tree_type, &max);
			out_op(op_gt);

			out_jfalse(cse->expr->spel);

			out_label(skip);
			free(skip);

		}else{
			out_dup();
			out_push_iv(cse->expr->tree_type, &iv);

			out_op(op_eq);

			out_jtrue(cse->expr->spel);
		}
	}

	out_pop(); /* free the value we switched on asap */

	out_push_lbl(tdefault ? tdefault->expr->spel : s->lbl_break, 0, NULL);
	out_jmp();

	/* out-stack must be empty from here on */

	gen_stmt(s->lhs); /* the actual code inside the switch */

	out_label(s->lbl_break);
}

int switch_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void mutate_stmt_switch(stmt *s)
{
	s->f_passable = switch_passable;
}
