#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_switch.h"
#include "../sue.h"
#include "../../util/alloc.h"
#include "../out/lbl.h"
#include "../type_is.h"
#include "../../util/dynarray.h"

#define ITER_SWITCH(sw, iter) \
	for(iter = sw->bits.switch_.cases; iter && *iter; iter++)

const char *str_stmt_switch()
{
	return "switch";
}

/* checks case duplicates, not default */
static void fold_switch_dups(stmt *sw)
{
	typedef int (*qsort_f)(const void *, const void *);

	struct
	{
		numeric start, end;
		stmt *cse;
	} *vals;
	size_t n = dynarray_count(sw->bits.switch_.cases);
	size_t i = 0;
	stmt **titer;

	if(n == 0)
		return;

	vals = malloc(n * sizeof *vals);

	/* gather all switch values */
	ITER_SWITCH(sw, titer){
		stmt *cse = *titer;

		vals[i].cse = cse;

		const_fold_integral(cse->expr, &vals[i].start);

		if(stmt_kind(cse, case_range))
			const_fold_integral(cse->expr2, &vals[i].end);
		else
			memcpy(&vals[i].end, &vals[i].start, sizeof vals[i].end);

		i++;
	}

	/* sort vals for comparison */
	qsort(vals, n,
			sizeof(*vals), (qsort_f)numeric_cmp); /* struct layout guarantees this */

	for(i = 1; i < n; i++){
		const long last_prev  = vals[i-1].end.val.i;
		const long first_this = vals[i].start.val.i;

		if(last_prev >= first_this){
			char buf[WHERE_BUF_SIZ];
			const int overlap = vals[i  ].end.val.i != vals[i  ].start.val.i
				               || vals[i-1].end.val.i != vals[i-1].start.val.i;

			die_at(&vals[i-1].cse->where,
					"%s case statements %s %ld\n"
					"%s: note: other case",
					overlap ? "overlapping" : "duplicate",
					overlap ? "starting at" : "for",
					(long)vals[i].start.val.i,
					where_str_r(buf, &vals[i].cse->where));
		}
	}

	free(vals);
}

static void fold_switch_enum(
		stmt *sw, struct_union_enum_st *enum_sue)
{
	stmt **iter;
	char *marks;
	int nents;
	int midx;

	if(sw->bits.switch_.default_case)
		return;

	nents = enum_nentries(enum_sue);
	marks = umalloc(nents * sizeof *marks);

	/* warn if we switch on an enum bitmask */
	if(expr_attr_present(sw->expr, attr_enum_bitmask))
		cc1_warn_at(&sw->where, enum_switch_bitmask,
				"switch on enum with enum_bitmask attribute");

	/* for each case/default/case_range... */
	ITER_SWITCH(sw, iter){
		stmt *cse = *iter;
		integral_t v, w;

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

			for(midx = 0, mi = enum_sue->members; *mi; midx++, mi++){
				enum_member *m = (*mi)->enum_member;

				if(v == const_fold_val_i(m->val))
					marks[midx]++, found = 1;
			}

			if(!found)
				cc1_warn_at(&cse->where,
						enum_switch_imposter,
						"'case %ld' not a member of enum %s",
						(long)v, enum_sue->spel);
		}
	}

	for(midx = 0; midx < nents; midx++)
		if(!marks[midx])
			cc1_warn_at(&sw->where, switch_enum,
					"enum %s::%s not handled in switch",
					enum_sue->anon ? "" : enum_sue->spel,
					enum_sue->members[midx]->enum_member->spel);

	free(marks);
}

void fold_stmt_and_add_to_curswitch(stmt *cse)
{
	stmt *sw = cse->parent;

	fold_stmt(cse->lhs); /* compound */

	if(!sw)
		die_at(&cse->where, "%s not inside switch", cse->f_str());

	if(cse->expr){
		/* promote to the controlling statement */
		fold_insert_casts(sw->expr->tree_type, &cse->expr, cse->symtab);

		dynarray_add(&sw->bits.switch_.cases, cse);
	}else{
		if(sw->bits.switch_.default_case){
			char buf[WHERE_BUF_SIZ];

			die_at(&cse->where,
					"duplicate default statement\n"
					"%s: note: other default here",
					where_str_r(buf, &sw->bits.switch_.default_case->where));

			return;
		}
		sw->bits.switch_.default_case = cse;
	}

	/* we are compound, copy some attributes */
	cse->kills_below_code = cse->lhs->kills_below_code;
	/* TODO: copy ->freestanding? */
}

static void fold_switch_scopechecks(stmt *sw)
{
	stmt **iter;

	ITER_SWITCH(sw, iter){
		stmt *to = *iter;

		fold_check_scope_entry(&to->where, "case inside", sw->symtab, to->symtab);
	}
}

void fold_stmt_switch(stmt *s)
{
	FOLD_EXPR(s->expr, s->symtab);

	fold_check_expr(s->expr, FOLD_CHK_INTEGRAL, "switch");

	/* default integer promotions */
	expr_promote_default(&s->expr, s->symtab);

	/* this folds sub-statements,
	 * causing case: and default: to add themselves to
	 * ->parent->bits.switch_.cases
	 */
	fold_stmt(s->lhs);

	/* check for dups */
	fold_switch_dups(s);

	fold_switch_scopechecks(s);

	/* check for an enum */
	{
		struct_union_enum_st *sue = type_is_enum(
					s->expr->tree_type);

		if(sue)
			fold_switch_enum(s, sue);
	}
}

void gen_stmt_switch(const stmt *s, out_ctx *octx)
{
	stmt **iter, *pdefault;
	out_blk *blk_switch_end = out_blk_new(octx, "switch_fin");
	const out_val *cmp_with;
	struct out_dbg_lbl *el[2][2];

	stmt_init_blks(s, NULL, blk_switch_end);

	flow_gen(s->flow, s->symtab, el, octx);

	cmp_with = gen_expr(s->expr, octx);

	for(iter = s->bits.switch_.cases; iter && *iter; iter++){
		stmt *cse = *iter;
		numeric iv;
		out_blk *blk_cancel = out_blk_new(octx, "case_next");

		const_fold_integral(cse->expr, &iv);

		/* create the case blocks here,
		 * since we need the jumps before we code-gen them */
		cse->bits.case_blk = out_blk_new(octx, "case");

		if(stmt_kind(cse, case_range)){
			numeric max;
			const out_val *this_case[2];
			out_blk *blk_test2 = out_blk_new(octx, "range_true");

			/* TODO: proper signed/unsiged format - out_op() */
			const_fold_integral(cse->expr2, &max);

			this_case[0] = out_new_num(octx, cse->expr->tree_type, &iv);

			out_val_retain(octx, cmp_with);
			out_ctrl_branch(octx,
					out_op(octx, op_lt, cmp_with, this_case[0]),
					blk_cancel,
					blk_test2);

			out_current_blk(octx, blk_test2);
			this_case[1] = out_new_num(octx, cse->expr2->tree_type, &max);

			out_val_retain(octx, cmp_with);
			out_ctrl_branch(octx,
					out_op(octx, op_gt, cmp_with, this_case[1]),
					blk_cancel,
					cse->bits.case_blk);

		}else{
			const out_val *this_case = out_new_num(octx, cse->expr->tree_type, &iv);

			out_val_retain(octx, cmp_with);
			out_ctrl_branch(octx,
				out_op(octx, op_eq, this_case, cmp_with),
				cse->bits.case_blk,
				blk_cancel);
		}

		out_current_blk(octx, blk_cancel);
		/* implicitly linked to next */
	}

	out_val_release(octx, cmp_with);

	pdefault = s->bits.switch_.default_case;
	if(pdefault)
		pdefault->bits.case_blk = out_blk_new(octx, "default");

	/* no matches - branch to default/end */
	out_ctrl_transfer(octx,
			pdefault ? pdefault->bits.case_blk : blk_switch_end,
			NULL, NULL);

	{
		out_blk *body = out_blk_new(octx, "switch_body");
		out_current_blk(octx, body);
		gen_stmt(s->lhs, octx); /* the actual code inside the switch */
		out_ctrl_transfer(octx, blk_switch_end, NULL, NULL);
		out_current_blk(octx, blk_switch_end);
	}

	flow_end(s->flow, s->symtab, el, octx);
}

void style_stmt_switch(const stmt *s, out_ctx *octx)
{
	stylef("switch(");
	IGNORE_PRINTGEN(gen_expr(s->expr, octx));
	stylef(")");
	gen_stmt(s->lhs, octx);
}

static int switch_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void init_stmt_switch(stmt *s)
{
	s->f_passable = switch_passable;
}
