#include <stdlib.h>

#include "ops.h"
#include "stmt_switch.h"
#include "../enum.h"
#include "../../util/alloc.h"

const char *str_stmt_switch()
{
	return "switch";
}

void fold_switch_enum(stmt *sw, type *enum_type)
{
	const int nents = enum_nentries(enum_type->enu);
	stmt **titer;
	char *marks = umalloc(nents * sizeof *marks);
	int midx;

	/* for each case/default/case_range... */
	for(titer = sw->codes; titer && *titer; titer++){
		stmt *cse = *titer;
		int v, w;

		if(cse->expr->expr_is_default)
			goto ret;

		v = cse->expr->val.iv.val;

		if(stmt_kind(cse, case_range))
			w = cse->expr2->val.iv.val;
		else
			w = v;

		for(; v <= w; v++){
			enum_member **mi;
			for(midx = 0, mi = enum_type->enu->members; *mi; midx++, mi++)
				if(v == (*mi)->val->val.iv.val)
					marks[midx]++;
		}
	}

	for(midx = 0; midx < nents; midx++)
		if(!marks[midx])
			cc1_warn_at(&sw->where, 0, WARN_SWITCH_ENUM, "enum %s not handled in switch", enum_type->enu->members[midx]->spel);

ret:
	free(marks);
}

void fold_stmt_switch(stmt *s)
{
	stmt *oldswstmt = curstmt_switch;
	stmt *oldflowstmt = curstmt_flow;
	type *typ;

	curstmt_switch = s;
	curstmt_flow   = s;

	s->lbl_break = asm_label_flow("switch");

	fold_expr(s->expr, s->symtab);

	fold_test_expr(s->expr, "switch");

	OPT_CHECK(s->expr, "constant expression in switch");

	fold_stmt(s->lhs);
	/* FIXME: check for duplicate case values and at most, 1 default */

	/* check for an enum */
	typ = s->expr->tree_type->type;
	if(typ->primitive == type_enum){
		UCC_ASSERT(typ->enu, "no enum for enum type");
		fold_switch_enum(s, typ);
	}

	curstmt_switch = oldswstmt;
	curstmt_flow   = oldflowstmt;
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

		UCC_ASSERT(cse->expr->expr_is_default || !(cse->expr->val.iv.suffix & VAL_UNSIGNED), "don's handle unsigned yet");

		if(stmt_kind(cse, case_range)){
			char *skip = asm_label_code("range_skip");
			asm_temp(1, "cmp rax, %d", cse->expr->val.iv.val);
			asm_temp(1, "j%s %s", is_unsigned ? "b" : "l", skip);
			asm_temp(1, "cmp rax, %d", cse->expr2->val.iv.val);
			asm_temp(1, "j%se %s", is_unsigned ? "b" : "l", cse->expr->spel);
			asm_label(skip);
			free(skip);
		}else if(cse->expr->expr_is_default){
			tdefault = cse;
		}else{
			/* FIXME: address-of, etc? */
			asm_temp(1, "cmp rax, %d", cse->expr->val.iv.val);
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
