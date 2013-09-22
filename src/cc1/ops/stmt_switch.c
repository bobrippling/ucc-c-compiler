#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_switch.h"
#include "../sue.h"
#include "../../util/alloc.h"
#include "../../util/dynarray.h"
#include "../out/lbl.h"

const char *str_stmt_switch()
{
	return "switch";
}

static void fold_switch_dups(stmt *sw)
{
	typedef int (*qsort_f)(const void *, const void *);

	int n = dynarray_count(sw->codes);
	struct
	{
		numeric start, end;
		stmt *cse;
	} *const vals = malloc(n * sizeof *vals);

	stmt **titer, *def = NULL;
	int i;

	/* gather all switch values */
	for(i = 0, titer = sw->codes; titer && *titer; titer++){
		stmt *cse = *titer;

		if(cse->expr->expr_is_default){
			if(def){
				char buf[WHERE_BUF_SIZ];

				die_at(&cse->where, "duplicate default statement (from %s)",
						where_str_r(buf, &def->where));
			}
			def = cse;
			n--;
			continue;
		}

		vals[i].cse = cse;

		const_fold_integral(cse->expr, &vals[i].start);

		if(stmt_kind(cse, case_range))
			const_fold_integral(cse->expr2, &vals[i].end);
		else
			memcpy(&vals[i].end, &vals[i].start, sizeof vals[i].end);

		i++;
	}

	/* sort vals for comparison */
	qsort(vals, n, sizeof(*vals), (qsort_f)numeric_cmp); /* struct layout guarantees this */

	for(i = 1; i < n; i++){
		const long last_prev  = vals[i-1].end.val.i;
		const long first_this = vals[i].start.val.i;

		if(last_prev >= first_this){
			char buf[WHERE_BUF_SIZ];
			const int overlap = vals[i  ].end.val.i != vals[i  ].start.val.i
				               || vals[i-1].end.val.i != vals[i-1].start.val.i;

			die_at(&vals[i-1].cse->where, "%s case statements %s %ld (from %s)",
					overlap ? "overlapping" : "duplicate",
					overlap ? "starting at" : "for",
					(long)vals[i].start.val.i,
					where_str_r(buf, &vals[i].cse->where));
		}
	}

	free(vals);
}

static void fold_switch_enum(stmt *sw, const type *enum_type)
{
	const int nents = enum_nentries(enum_type->sue);
	stmt **titer;
	char *const marks = umalloc(nents * sizeof *marks);
	int midx;

	/* for each case/default/case_range... */
	for(titer = sw->codes; titer && *titer; titer++){
		stmt *cse = *titer;
		integral_t v, w;

		if(cse->expr->expr_is_default)
			goto ret;

		fold_check_expr(cse->expr,
				FOLD_CHK_INTEGRAL | FOLD_CHK_CONST_I,
				"case value");
		v = const_fold_val_i(cse->expr);

		if(stmt_kind(cse, case_range)){
			fold_check_expr(cse->expr2,
					FOLD_CHK_INTEGRAL | FOLD_CHK_CONST_I,
					"case range value");

			w =  const_fold_val_i(cse->expr2);
		}else{
			w = v;
		}

		for(; v <= w; v++){
			sue_member **mi;
			int found = 0;

			for(midx = 0, mi = enum_type->sue->members; *mi; midx++, mi++){
				enum_member *m = (*mi)->enum_member;

				if(v == const_fold_val_i(m->val))
					marks[midx]++, found = 1;
			}

			if(!found)
				warn_at(&cse->where, "'case %ld' not not a member of enum %s",
						(long)v, enum_type->sue->spel);
		}
	}

	for(midx = 0; midx < nents; midx++)
		if(!marks[midx])
			cc1_warn_at(&sw->where, 0, WARN_SWITCH_ENUM,
					"enum %s::%s not handled in switch",
					enum_type->sue->anon ? "" : enum_type->sue->spel,
					enum_type->sue->members[midx]->enum_member->spel);

ret:
	free(marks);
}

void fold_stmt_switch(stmt *s)
{
	symtable *stab = s->symtab;

	flow_fold(s->flow, &stab);

	s->lbl_break = out_label_flow("switch");

	FOLD_EXPR(s->expr, stab);

	fold_check_expr(s->expr, FOLD_CHK_INTEGRAL, "switch");

	/* this folds sub-statements,
	 * causing case: and default: to add themselves to ->parent->codes,
	 * i.e. s->codes
	 */
	fold_stmt(s->lhs);

	/* check for dups */
	fold_switch_dups(s);

	/* check for an enum */
	{
		type_ref *r = type_ref_is_type(s->expr->tree_type, type_enum);

		if(r){
			const type *typ = r->bits.type;
			UCC_ASSERT(typ->sue, "no enum for enum type");
			fold_switch_enum(s, typ);

			/* warn if we switch on an enum bitmask */
			if(attr_present(typ->sue->attr, attr_enum_bitmask))
				warn_at(&s->where, "switch on enum with enum_bitmask attribute");
		}
	}
}

basic_blk *gen_stmt_switch(stmt *s, basic_blk *bb)
{
	stmt **titer, *tdefault;

	tdefault = NULL;

	bb = gen_expr(s->expr, bb);

	out_comment(bb, "switch on this");

	for(titer = s->codes; titer && *titer; titer++){
		stmt *cse = *titer;
		numeric iv;

		if(cse->expr->expr_is_default){
			tdefault = cse;
			continue;
		}

		const_fold_integral(cse->expr, &iv);

		UCC_ASSERT(cse->expr->expr_is_default || !(iv.suffix & VAL_UNSIGNED),
				"don't handle unsigned yet");

		if(stmt_kind(cse, case_range)){
			char *skip = out_label_code("range_skip");
			numeric max;

			/* TODO: proper signed/unsiged format - out_op(bb) */
			const_fold_integral(cse->expr2, &max);

			out_dup(bb);
			out_push_num(bb, cse->expr->tree_type, &iv);

			out_op(bb, op_lt);
			out_jtrue(bb, skip);

			out_dup(bb);
			out_push_num(bb, cse->expr2->tree_type, &max);
			out_op(bb, op_gt);

			out_jfalse(bb, cse->expr->bits.ident.spel);

			out_label(bb, skip);
			free(skip);

		}else{
			out_dup(bb);
			out_push_num(bb, cse->expr->tree_type, &iv);

			out_op(bb, op_eq);

			out_jtrue(bb, cse->expr->bits.ident.spel);
		}
	}

	out_pop(bb); /* free the value we switched on asap */

	out_push_lbl(bb, tdefault ? tdefault->expr->bits.ident.spel : s->lbl_break, 0);
	out_jmp(bb);

	/* out-stack must be empty from here on */

	bb = gen_stmt(s->lhs, bb); /* the actual code inside the switch */

	out_label(bb, s->lbl_break);

	return bb;
}

basic_blk *style_stmt_switch(stmt *s, basic_blk *bb)
{
	stylef("switch(");
	bb = gen_expr(s->expr, bb);
	stylef(")");
	bb = gen_stmt(s->lhs, bb);
	return bb;
}

static int switch_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void mutate_stmt_switch(stmt *s)
{
	s->f_passable = switch_passable;
}
