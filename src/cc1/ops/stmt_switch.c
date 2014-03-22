#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_switch.h"
#include "../sue.h"
#include "../../util/alloc.h"
#include "../out/lbl.h"
#include "../type_is.h"

#define ITER_SWITCH(sw, iter) \
	for(iter = sw->bits.switch_.cases; iter && iter->code; iter++)

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
	} *const vals = malloc(sw->bits.switch_.ncases * sizeof *vals);


	struct switch_case *titer;
	size_t i = 0;

	/* gather all switch values */
	ITER_SWITCH(sw, titer){
		stmt *cse = titer->code;

		vals[i].cse = cse;

		const_fold_integral(cse->expr, &vals[i].start);

		if(stmt_kind(cse, case_range))
			const_fold_integral(cse->expr2, &vals[i].end);
		else
			memcpy(&vals[i].end, &vals[i].start, sizeof vals[i].end);

		i++;
	}

	/* sort vals for comparison */
	qsort(vals, sw->bits.switch_.ncases,
			sizeof(*vals), (qsort_f)numeric_cmp); /* struct layout guarantees this */

	for(i = 1; i < sw->bits.switch_.ncases; i++){
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
	struct switch_case *iter;
	char *marks;
	int nents;
	int midx;

	if(sw->bits.switch_.default_case.code)
		return;

	nents = enum_nentries(enum_sue);
	marks = umalloc(nents * sizeof *marks);

	/* warn if we switch on an enum bitmask */
	if(expr_attr_present(sw->expr, attr_enum_bitmask))
		warn_at(&sw->where, "switch on enum with enum_bitmask attribute");

	/* for each case/default/case_range... */
	ITER_SWITCH(sw, iter){
		stmt *cse = iter->code;
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
				warn_at(&cse->where, "'case %ld' not a member of enum %s",
						(long)v, enum_sue->spel);
		}
	}

	for(midx = 0; midx < nents; midx++)
		if(!marks[midx])
			cc1_warn_at(&sw->where, 0, WARN_SWITCH_ENUM,
					"enum %s::%s not handled in switch",
					enum_sue->anon ? "" : enum_sue->spel,
					enum_sue->members[midx]->enum_member->spel);

	free(marks);
}

void fold_stmt_and_add_to_curswitch(stmt *cse, char **lbl)
{
	stmt *sw = cse->parent;
	struct switch_case *this_case;

	fold_stmt(cse->lhs); /* compound */

	if(!sw)
		die_at(&cse->where, "%s not inside switch", cse->f_str());

	if(*lbl){
		/* promote to the controlling statement */
		fold_insert_casts(sw->expr->tree_type, &cse->expr, cse->symtab);

		sw->bits.switch_.ncases++;
		sw->bits.switch_.cases = urealloc(
				sw->bits.switch_.cases,
				sizeof(sw->bits.switch_.cases[0]) * (1 + sw->bits.switch_.ncases),
				sizeof(sw->bits.switch_.cases[0]) * sw->bits.switch_.ncases);

		this_case = &sw->bits.switch_.cases[sw->bits.switch_.ncases - 1];
	}else{
		this_case = &sw->bits.switch_.default_case;
		if(this_case->code){
			char buf[WHERE_BUF_SIZ];

			die_at(&cse->where,
					"duplicate default statement\n"
					"%s: note: other default here",
					where_str_r(buf, &this_case->code->where));

			return;
		}
		*lbl = out_label_case(CASE_DEF, 0);
	}

	this_case->code = cse;
	this_case->lbl = *lbl;

	/* we are compound, copy some attributes */
	cse->kills_below_code = cse->lhs->kills_below_code;
	/* TODO: copy ->freestanding? */
}

void fold_stmt_switch(stmt *s)
{
	s->lbl_break = out_label_flow("switch");

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

	/* check for an enum */
	{
		struct_union_enum_st *sue = type_is_s_or_u_or_e(
					s->expr->tree_type);

		if(sue && sue->primitive == type_enum)
			fold_switch_enum(s, sue);
	}
}

void gen_stmt_switch(stmt *s, out_ctx *octx)
{
	struct switch_case *iter, *pdefault;

	gen_expr(s->expr);

	out_comment("switch on this");

	for(iter = s->bits.switch_.cases; iter && iter->code; iter++){
		stmt *cse = iter->code;
		numeric iv;

		const_fold_integral(cse->expr, &iv);

		if(stmt_kind(cse, case_range)){
			char *skip = out_label_code("range_skip");
			numeric max;

			/* TODO: proper signed/unsiged format - out_op() */
			const_fold_integral(cse->expr2, &max);

			out_dup();
			out_push_num(cse->expr->tree_type, &iv);

			out_op(op_lt);
			out_jtrue(skip);

			out_dup();
			out_push_num(cse->expr2->tree_type, &max);
			out_op(op_gt);

			out_jfalse(iter->lbl);

			out_label(skip);
			free(skip);

		}else{
			out_dup();
			out_push_num(cse->expr->tree_type, &iv);

			out_op(op_eq);

			out_jtrue(iter->lbl);
		}
	}

	out_pop(); /* free the value we switched on asap */

	pdefault = &s->bits.switch_.default_case;

	out_push_lbl(pdefault->code
			? pdefault->lbl
			: s->lbl_break, 0);

	out_jmp();

	/* out-stack must be empty from here on */

	gen_stmt(s->lhs); /* the actual code inside the switch */

	out_label(s->lbl_break);
}

void style_stmt_switch(stmt *s)
{
	stylef("switch(");
	gen_expr(s->expr);
	stylef(")");
	gen_stmt(s->lhs);
}

static int switch_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void init_stmt_switch(stmt *s)
{
	s->f_passable = switch_passable;
}
