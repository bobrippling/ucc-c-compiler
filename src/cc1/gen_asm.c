#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "macros.h"
#include "asm.h"
#include "../util/platform.h"
#include "sym.h"
#include "gen_asm.h"
#include "../util/util.h"
#include "const.h"
#include "struct.h"

char *curfunc_lblfin; /* extern */

void gen_expr(expr *e, symtable *stab)
{
	e->f_gen(e, stab);
}

void walk_tree(tree *t)
{
	switch(t->type){
		case stat_switch:
		{
			tree **titer, *tdefault;
			int is_unsigned = t->expr->tree_type->type->spec & spec_unsigned;

			tdefault = NULL;

			gen_expr(t->expr, t->symtab);
			asm_temp(1, "pop rax ; switch on this");

			for(titer = t->codes; titer && *titer; titer++){
				tree *cse = *titer;

				UCC_ASSERT(cse->expr->expr_is_default || !(cse->expr->val.iv.suffix & VAL_UNSIGNED), "don't handle unsigned yet");

				if(cse->type == stat_case_range){
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
				asm_temp(1, "jmp %s", t->lblfin);

			walk_tree(t->lhs); /* the actual code inside the switch */

			asm_label(t->lblfin);
			break;
		}

		case stat_case:
		case stat_default:
		case stat_label:
		case stat_case_range:
			asm_label(t->expr->spel);
			walk_tree(t->lhs); /* the code-part of the compound statement */
			break;

		case stat_goto:
		case stat_break:
			asm_temp(1, "jmp %s", t->expr->spel);
			break;

		case stat_if:
		{
			char *lbl_else = asm_label_code("else");
			char *lbl_fi   = asm_label_code("fi");

			gen_expr(t->expr, t->symtab);

			asm_temp(1, "pop rax");
			asm_temp(1, "test rax, rax");
			asm_temp(1, "jz %s", lbl_else);
			walk_tree(t->lhs);
			asm_temp(1, "jmp %s", lbl_fi);
			asm_label(lbl_else);
			if(t->rhs)
				walk_tree(t->rhs);
			asm_label(lbl_fi);

			free(lbl_else);
			free(lbl_fi);
			break;
		}

		case stat_for:
		{
			char *lbl_for;

			lbl_for = asm_label_code("for");

			if(t->flow->for_init){
				gen_expr(t->flow->for_init, t->symtab);
				asm_temp(1, "pop rax ; unused for init");
			}

			asm_label(lbl_for);
			if(t->flow->for_while){
				gen_expr(t->flow->for_while, t->symtab);

				asm_temp(1, "pop rax");
				asm_temp(1, "test rax, rax");
				asm_temp(1, "jz %s", t->lblfin);
			}

			walk_tree(t->lhs);
			if(t->flow->for_inc){
				gen_expr(t->flow->for_inc, t->symtab);
				asm_temp(1, "pop rax ; unused for inc");
			}

			asm_temp(1, "jmp %s", lbl_for);

			asm_label(t->lblfin);

			free(lbl_for);
			break;
		}

		case stat_while:
		{
			char *lbl_start;

			lbl_start = asm_label_code("while");

			asm_label(lbl_start);
			gen_expr(t->expr, t->symtab);
			asm_temp(1, "pop rax");
			asm_temp(1, "test rax, rax");
			asm_temp(1, "jz %s", t->lblfin);
			walk_tree(t->lhs);
			asm_temp(1, "jmp %s", lbl_start);
			asm_label(t->lblfin);

			free(lbl_start);
			break;
		}

		case stat_do:
		{
			char *lbl_start;

			lbl_start = asm_label_code("do");

			asm_label(lbl_start);
			walk_tree(t->lhs);

			gen_expr(t->expr, t->symtab);
			asm_temp(1, "pop rax");
			asm_temp(1, "test rax, rax");
			asm_temp(1, "jnz %s", lbl_start);

			free(lbl_start);
			asm_label(t->lblfin);
			break;
		}

		case stat_return:
			if(t->expr){
				gen_expr(t->expr, t->symtab);
				asm_temp(1, "pop rax ; return");
			}
			asm_temp(1, "jmp %s", curfunc_lblfin);
			break;

		case stat_expr:
			gen_expr(t->expr, t->symtab);
			if((fopt_mode & FOPT_ENABLE_ASM) == 0
			|| !t->expr
			|| expr_kind(t->expr, funcall)
			|| !t->expr->spel
			|| strcmp(t->expr->spel, ASM_INLINE_FNAME))
			{
				asm_temp(1, "pop rax ; unused expr");
			}
			break;

		case stat_code:
			if(t->codes){
				tree **titer;
				decl **diter;

				/* declare statics */
				for(diter = t->decls; diter && *diter; diter++){
					decl *d = *diter;
					if(d->type->spec & (spec_static | spec_extern))
						gen_asm_global(d);
				}

				for(titer = t->codes; *titer; titer++)
					walk_tree(*titer);
			}
			break;

		case stat_noop:
			asm_temp(1, "; noop");
			break;
	}
}

void gen_asm_global(decl *d)
{
	if(d->type->spec & spec_extern){
		/* should be fine... */
		asm_tempf(cc_out[SECTION_BSS], 0, "extern %s", d->spel);
		return;
	}

	if(d->func_code){
		int offset;

		asm_label(d->spel);
		asm_temp(1, "push rbp");
		asm_temp(1, "mov rbp, rsp");

		curfunc_lblfin = asm_label_code(d->spel);

		if((offset = d->func_code->symtab->auto_total_size))
			asm_temp(1, "sub rsp, %d", offset);

		walk_tree(d->func_code);

		asm_label(curfunc_lblfin);
		if(offset)
			asm_temp(1, "add rsp, %d", offset);

		asm_temp(1, "leave");
		asm_temp(1, "ret");
		free(curfunc_lblfin);

	}else if(d->arrayinit){
		asm_declare_array(SECTION_DATA, d->arrayinit->label, d->arrayinit);

	}else if(d->init && !const_expr_is_zero(d->init)){
		asm_declare_single(cc_out[SECTION_DATA], d);

	}else{
		asm_tempf(cc_out[SECTION_BSS], 0, "%s res%c %d", d->spel, asm_type_ch(d), decl_size(d));
	}
}

void gen_asm(symtable *globs)
{
	decl **diter;
	for(diter = globs->decls; diter && *diter; diter++){
		decl *d = *diter;

		if(d->ignore)
			continue;

		if(!(d->type->spec & spec_static) && !(d->type->spec & spec_extern))
			asm_temp(0, "global %s", d->spel);

		gen_asm_global(d);
	}
}
