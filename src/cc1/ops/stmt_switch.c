#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_switch.h"
#include "../sue.h"
#include "../../util/alloc.h"
#include "../../util/dynarray.h"

const char *str_stmt_switch()
{
	return "switch";
}

#warning FIXME #3
/* FIXME: merge with enum checking? */
void fold_switch_dups(stmt *sw)
{
	const int n = dynarray_count((void **)sw->codes);
	struct
	{
		intval v;
		stmt *cse;
	} *const vals = malloc(n * sizeof *vals);

	stmt **titer;
	int i;

	/* gather all switch values */
	for(i = 0, titer = sw->codes; titer && *titer; titer++, i++){
		stmt *cse = *titer;

		if(cse->expr->expr_is_default)
			continue;

		if(stmt_kind(cse, case_range))
			ICE("TODO: dup checking on switch ranges");

		const_fold_need_val(cse->expr, &vals[i].v);
		vals[i].cse = cse;
	}

	/* sort vals for comparison */
	qsort(vals, n, sizeof(*vals), (intval_cmp_cast)intval_cmp); /* struct layout guarantees this */

	for(i = 1; i < n; i++)
		if(vals[i-1].v.val == vals[i].v.val){
			char buf[WHERE_BUF_SIZ];

			DIE_AT(&vals[i-1].cse->where, "duplicate case statement %ld (from %s)",
					vals[i].v.val, where_str_r(buf, &vals[i].cse->where));
		}

	free(vals);
}

void fold_switch_enum(stmt *sw, type *enum_type)
{
	const int nents = enum_nentries(enum_type->sue);
	stmt **titer;
	char *const marks = umalloc(nents * sizeof *marks);
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

	s->lbl_break = asm_label_flow("switch");

	fold_expr(s->expr, test_symtab);

	fold_need_expr(s->expr, "switch", 0);

	OPT_CHECK(s->expr, "constant expression in switch");

	/* this folds sub-statements,
	 * causing case: and default: to add themselves to ->parent->codes,
	 * i.e. s->codes
	 */
	fold_stmt(s->lhs);

	/* check for dups */
	fold_switch_dups(s);

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
	int is_unsigned = !s->expr->tree_type->type->is_signed;

	tdefault = NULL;

	gen_expr(s->expr, s->symtab);
	asm_temp(1, "pop rax ; switch on this");

	for(titer = s->codes; titer && *titer; titer++){
		stmt *cse = *titer;

		UCC_ASSERT(cse->expr->expr_is_default || !(cse->expr->bits.iv.suffix & VAL_UNSIGNED), "don's handle unsigned yet");

		if(stmt_kind(cse, case_range)){
			char *skip = asm_label_code("range_skip");
			intval min, max;

			const_fold_need_val(cse->expr,  &min);
			const_fold_need_val(cse->expr2, &max);

			/* TODO: proper signed/unsiged format */
			asm_temp(1, "cmp rax, %ld", min.val);
			asm_temp(1, "j%s %s", is_unsigned ? "b" : "l", skip);
			asm_temp(1, "cmp rax, %ld", max.val);
			asm_temp(1, "j%se %s", is_unsigned ? "b" : "l", cse->expr->spel);
			asm_label(skip);
			free(skip);
		}else if(cse->expr->expr_is_default){
			tdefault = cse;
		}else{
			/* FIXME: address-of, etc? */
			intval iv;

			const_fold_need_val(cse->expr, &iv);

			asm_temp(1, "cmp rax, %ld", iv.val);
			asm_temp(1, "je %s", cse->expr->spel);
		}
	}

	if(tdefault)
		asm_temp(1, "jmp %s", tdefault->expr->spel);
	else
		asm_temp(1, "jmp %s", s->lbl_break);

	gen_stmt(s->lhs); /* the actual code inside the switch */

	asm_label(s->lbl_break);
}

int switch_passable(stmt *s)
{
	return fold_passable(s->lhs);
}

void mutate_stmt_switch(stmt *s)
{
	s->f_passable = switch_passable;
}
