#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"
#include "tree.h"
#include "cc1.h"
#include "macros.h"
#include "asm.h"
#include "../util/platform.h"
#include "sym.h"
#include "asm_op.h"
#include "gen_asm.h"
#include "../util/util.h"
#include "const.h"
#include "struct.h"

static char *curfunc_lblfin;

void gen_asm_global_var(decl *d);

void asm_ax_to_store(expr *store, symtable *stab)
{
	switch(store->type){
		case expr_identifier:
			asm_sym(ASM_SET, store->sym, "rax");
			return;

		case expr_op:
			switch(store->op){
				case op_deref:
					/* a dereference */
					asm_temp(1, "push rax ; save val");

					walk_expr(store->lhs, stab); /* skip over the *() bit */
					/* pointer on stack */

					/* move `pop` into `pop` */
					asm_temp(1, "pop rax ; ptr");
					asm_temp(1, "pop rbx ; val");
					asm_temp(1, "mov [rax], rbx");
					return;

				case op_struct_ptr:
				case op_struct_dot:
					ICE("TODO: op_struct_*");
					asm_temp(1, "pop rbx ; struct addr");
					asm_temp(1, "sub rbx, %d ; offset of member %s", -1/*struct_member_offset(store)*/, store->rhs->spel);
					asm_temp(1, "pop rax ; saved val");
					asm_temp(1, "mov [rbx], rax");
					break;
					return;

				default:
					break;
			}


		default:
			break;
	}

	ICE("asm_ax_to_store: invalid store expression %s", expr_to_str(store->type));
}

void gen_assign(expr *e, symtable *stab)
{
	if(e->assign_is_post){
		/* if this is the case, ->rhs->lhs is ->lhs, and ->rhs is an addition/subtraction of 1 * something */
		walk_expr(e->lhs, stab);
		asm_temp(1, "; save previous for post assignment");
	}

	walk_expr(e->rhs, stab);
#ifdef USE_MOVE_RAX_RSP
	asm_temp(1, "mov rax, [rsp]");
#endif

	/* store back to the sym's home */
	asm_ax_to_store(e->lhs, stab);

	if(e->assign_is_post){
		asm_temp(1, "pop rax ; the value from ++/--");
		asm_temp(1, "mov rax, [rsp] ; the value we saved");
	}
}

void walk_expr(expr *e, symtable *stab)
{
	switch(e->type){
		case expr_cast:
			/* ignore the lhs, it's just a type spec */
			/* FIXME: size changing? */
			walk_expr(e->rhs, stab);
			break;

		case expr_comma:
			walk_expr(e->lhs, stab);
			asm_temp(1, "pop rax ; unused comma expr");
			walk_expr(e->rhs, stab);
			break;

		case expr_if:
		{
			char *lblfin, *lblelse;
			lblfin  = asm_label_code("ifexpa");
			lblelse = asm_label_code("ifexpb");

			walk_expr(e->expr, stab);
			asm_temp(1, "pop rax");
			asm_temp(1, "test rax, rax");
			asm_temp(1, "jz %s", lblelse);
			walk_expr(e->lhs ? e->lhs : e->expr, stab);
			asm_temp(1, "jmp %s", lblfin);
			asm_label(lblelse);
			walk_expr(e->rhs, stab);
			asm_label(lblfin);

			free(lblfin);
			free(lblelse);
			break;
		}

		case expr_identifier:
			/* if it's an array, lea, else, load */
			asm_sym(e->sym->decl->arraysizes ? ASM_LEA : ASM_LOAD, e->sym, "rax");
			asm_temp(1, "push rax");
			break;

		case expr_val:
			/*asm_new(asm_load_val, &e->val);*/
			/*asm_temp(1, "mov rax, %d", e->val);*/
			fputs("\tmov rax, ", cc_out[SECTION_TEXT]);
			asm_out_intval(cc_out[SECTION_TEXT], &e->val.i);
			fputc('\n', cc_out[SECTION_TEXT]);
			asm_temp(1, "push rax");
			break;

		case expr_op:
			asm_operate(e, stab);
			break;

		case expr_assign:
			gen_assign(e, stab);
			break;

		case expr_funcall:
		{
			/* currently only identifiers are allowed */
			const char *const fname = e->expr->spel;
			expr **iter;
			int nargs = 0;

			if(fopt_mode & FOPT_ENABLE_ASM && !strcmp(fname, ASM_INLINE_FNAME)){
				const char *str;
				expr *arg1;
				int i;

				if(!e->funcargs || e->funcargs[1] || e->funcargs[0]->type != expr_addr)
					die_at(&e->where, "invalid __asm__ arguments");

				arg1 = e->funcargs[0];
				str = arg1->array_store->data.str;
				for(i = 0; i < arg1->array_store->len - 1; i++){
					char ch = str[i];
					if(ch != '\n' && !isprint(ch))
invalid:
						die_at(&arg1->where, "invalid __asm__ string (character %d)", ch);
				}

				if(str[i])
					goto invalid;

				asm_temp(0, "; start manual __asm__");
				fprintf(cc_out[SECTION_TEXT], "%s\n", arg1->array_store->data.str);
				asm_temp(0, "; end manual __asm__");
				break;
			}
			/* continue with normal funcall */

			if(e->funcargs){
				/* need to push on in reverse order */
				for(iter = e->funcargs; *iter; iter++);
				for(iter--; iter >= e->funcargs; iter--){
					walk_expr(*iter, stab);
					nargs++;
				}
			}

			/*asm_new(asm_call, e->spel);*/
			asm_temp(1, "call %s", fname);

			if(nargs)
				asm_temp(1, "add rsp, %d ; %d arg%s",
						nargs * platform_word_size(),
						nargs,
						nargs == 1 ? "" : "s");

			asm_temp(1, "push rax ; ret");
			break;
		}

		case expr_addr:
			if(e->array_store)
				asm_temp(1, "mov rax, %s", e->array_store->label);
			else
				asm_sym(ASM_LEA, e->sym, "rax");

			asm_temp(1, "push rax");
			break;

		case expr_sizeof:
		{
			decl *d = e->expr->tree_type;
			asm_temp(1, "push %d ; sizeof %s%s", decl_size(d), e->expr->expr_is_sizeof ? "type " : "", decl_to_str(d));
			break;
		}
	}
}

void walk_tree(tree *t)
{
	switch(t->type){
		case stat_switch:
		{
			tree **titer, *tdefault;
			int is_unsigned = t->expr->tree_type->type->spec & spec_unsigned;

			tdefault = NULL;

			walk_expr(t->expr, t->symtab);
			asm_temp(1, "pop rax ; switch on this");

			for(titer = t->codes; titer && *titer; titer++){
				tree *cse = *titer;

				if(cse->type == stat_case_range){
					char *skip = asm_label_code("range_skip");
					asm_temp(1, "cmp rax, %d", cse->lhs->expr->val.i);
					asm_temp(1, "j%s %s", is_unsigned ? "b" : "l", skip);
					asm_temp(1, "cmp rax, %d", cse->rhs->expr->val.i);
					asm_temp(1, "j%se %s", is_unsigned ? "b" : "l", cse->lhs->expr->spel);
					asm_label(skip);
					free(skip);
				}else if(cse->expr->expr_is_default){
					tdefault = cse;
				}else{
					/* FIXME: address-of, etc? */
					asm_temp(1, "cmp rax, %d", cse->expr->val.i);
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
			asm_label(t->expr->spel);
			break;
		case stat_case_range:
			asm_label(t->lhs->expr->spel);
			break;

		case stat_goto:
		case stat_break:
			asm_temp(1, "jmp %s", t->expr->spel);
			break;

		case stat_if:
		{
			char *lbl_else = asm_label_code("else");
			char *lbl_fi   = asm_label_code("fi");

			walk_expr(t->expr, t->symtab);

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
				walk_expr(t->flow->for_init, t->symtab);
				asm_temp(1, "pop rax ; unused for init");
			}

			asm_label(lbl_for);
			if(t->flow->for_while){
				walk_expr(t->flow->for_while, t->symtab);

				asm_temp(1, "pop rax");
				asm_temp(1, "test rax, rax");
				asm_temp(1, "jz %s", t->lblfin);
			}

			walk_tree(t->lhs);
			if(t->flow->for_inc){
				walk_expr(t->flow->for_inc, t->symtab);
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
			walk_expr(t->expr, t->symtab);
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

			walk_expr(t->expr, t->symtab);
			asm_temp(1, "pop rax");
			asm_temp(1, "test rax, rax");
			asm_temp(1, "jnz %s", lbl_start);

			free(lbl_start);
			asm_label(t->lblfin);
			break;
		}

		case stat_return:
			if(t->expr){
				walk_expr(t->expr, t->symtab);
				asm_temp(1, "pop rax ; return");
			}
			asm_temp(1, "jmp %s", curfunc_lblfin);
			break;

		case stat_expr:
			walk_expr(t->expr, t->symtab);
			if((fopt_mode & FOPT_ENABLE_ASM) == 0 ||
					!t->expr ||
					t->expr->type != expr_funcall ||
					strcmp(t->expr->expr->spel, ASM_INLINE_FNAME))
				asm_temp(1, "pop rax ; unused expr");
			break;

		case stat_code:
			if(t->codes){
				tree **titer;
				decl **diter;

				/* declare statics */
				for(diter = t->decls; diter && *diter; diter++){
					decl *d = *diter;
					if(d->type->spec & (spec_static | spec_extern))
						gen_asm_global_var(d);
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

void gen_asm_func(decl *d)
{
	function *f = d->func;
	if(f->code){
		int offset;

		asm_label(d->spel);
		asm_temp(1, "push rbp");
		asm_temp(1, "mov rbp, rsp");

		curfunc_lblfin = asm_label_code(d->spel);

		if((offset = f->code->symtab->auto_total_size))
			asm_temp(1, "sub rsp, %d", offset);

		walk_tree(f->code);

		asm_label(curfunc_lblfin);
		if(offset)
			asm_temp(1, "add rsp, %d", offset);

		asm_temp(1, "leave");
		asm_temp(1, "ret");
		free(curfunc_lblfin);
	}else if(d->type->spec & spec_extern){
		asm_temp(0, "extern %s", d->spel);
	}
}

void gen_asm_global_var(decl *d)
{
	if(d->type->spec & spec_extern){
		/* should be fine... */
		asm_tempf(cc_out[SECTION_BSS], 0, "extern %s", d->spel);
		return;
	}

	if(d->arrayinit){
		asm_declare_array(SECTION_DATA, d->arrayinit->label, d->arrayinit);

	}else if(d->init && !const_expr_is_zero(d->init)){
		asm_declare_single(cc_out[SECTION_DATA], d);

	}else{
		int arraylen = 1;
		int i;

		for(i = 0; d->arraysizes && d->arraysizes[i]; i++)
			arraylen = (i + 1) * d->arraysizes[i]->val.i.val;

		/* TODO: check that i+1 is correct for the order here */

		asm_tempf(cc_out[SECTION_BSS], 0, "%s res%c %d", d->spel, asm_type_ch(d), arraylen);
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

		if(d->func)
			gen_asm_func(d);
		else
			gen_asm_global_var(d);
	}
}
