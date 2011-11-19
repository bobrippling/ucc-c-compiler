#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "macros.h"
#include "asm.h"
#include "../util/platform.h"
#include "sym.h"
#include "asm_op.h"
#include "gen_asm.h"
#include "../util/util.h"

static char *curfunc_lblfin;

void asm_ax_to_store(expr *store, symtable *stab)
{
	if(store->type == expr_identifier){
		asm_sym(ASM_SET, store->sym, "rax");

	}else if(store->type == expr_op && store->op == op_deref){
		/* a dereference */
		asm_temp(1, "push rax ; save val");

		walk_expr(store->lhs, stab); /* skip over the *() bit */

		/* move `pop` into `pop` */
		asm_temp(1, "pop rax ; ptr");
		asm_temp(1, "pop rbx ; val");
		asm_temp(1, "mov [rax], rbx");
	}else{
		ICE("asm_ax_to_store: invalid store expression");
	}
}

void gen_assign(expr *e, symtable *stab)
{
	if(e->assign_is_post){
		/* if this is the case, ->rhs->lhs is ->lhs, and ->rhs is an addition/subtraction of 1 * something */
		walk_expr(e->lhs, stab);
		asm_temp(1, "; save previous for post assignment");
	}

	walk_expr(e->rhs, stab);

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
			lblfin  = asm_code_label("ifexpa");
			lblelse = asm_code_label("ifexpb");

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
			asm_temp(1, "mov rax, %d", e->val);
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
			expr **iter;
			int nargs = 0;

			if(e->funcargs){
				/* need to push on in reverse order */
				for(iter = e->funcargs; *iter; iter++);
				for(iter--; iter >= e->funcargs; iter--){
					walk_expr(*iter, stab);
					nargs++;
				}
			}

			asm_new(asm_call, e->spel);
			if(nargs)
				asm_temp(1, "add rsp, %d ; %d arg%s",
						nargs * platform_word_size(),
						nargs,
						nargs == 1 ? "":"s");

			asm_temp(1, "push rax ; ret");
			break;
		}

		case expr_addr:
			asm_sym(ASM_LEA, e->sym, "rax");
			asm_temp(1, "push rax");
			break;

		case expr_sizeof:
			asm_temp(1, "push %d ; sizeof type %s", decl_size(e->tree_type), decl_to_str(e->tree_type));
			break;

		case expr_str:
			asm_temp(1, "mov rax, %s", e->sym->decl->spel);
			asm_temp(1, "push rax");
			break;
	}
}

void walk_tree(tree *t)
{
	switch(t->type){
		case stat_break:
			ICE("no break code yet");
			break;

		case stat_goto:
			asm_temp(1, "goto %s", t->expr->spel);
			break;

		case stat_if:
		{
			char *lbl_else = asm_code_label("else");
			char *lbl_fi   = asm_code_label("fi");

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
			char *lbl_for, *lbl_fin;

			lbl_for = asm_code_label("for");
			lbl_fin = asm_code_label("for_fin");

			if(t->flow->for_init){
				walk_expr(t->flow->for_init, t->symtab);
				asm_temp(1, "pop rax ; unused for init");
			}

			asm_label(lbl_for);
			if(t->flow->for_while){
				walk_expr(t->flow->for_while, t->symtab);

				asm_temp(1, "pop rax");
				asm_temp(1, "test rax, rax");
				asm_temp(1, "jz %s", lbl_fin);
			}

			walk_tree(t->lhs);
			if(t->flow->for_inc){
				walk_expr(t->flow->for_inc, t->symtab);
				asm_temp(1, "pop rax ; unused for inc");
			}

			asm_temp(1, "jmp %s", lbl_for);

			asm_label(lbl_fin);

			free(lbl_for);
			free(lbl_fin);
			break;
		}

		case stat_while:
		{
			char *lbl_start, *lbl_fin;

			lbl_start = asm_code_label("while");
			lbl_fin   = asm_code_label("while_fin");

			asm_label(lbl_start);
			walk_expr(t->expr, t->symtab);
			asm_temp(1, "pop rax");
			asm_temp(1, "test rax, rax");
			asm_temp(1, "jz %s", lbl_fin);
			walk_tree(t->lhs);
			asm_temp(1, "jmp %s", lbl_start);
			asm_label(lbl_fin);

			free(lbl_start);
			free(lbl_fin);
			break;
		}

		case stat_do:
		{
			char *lbl_start;

			lbl_start = asm_code_label("do");

			asm_label(lbl_start);
			walk_tree(t->lhs);

			walk_expr(t->expr, t->symtab);
			asm_temp(1, "pop rax");
			asm_temp(1, "test rax, rax");
			asm_temp(1, "jnz %s", lbl_start);

			free(lbl_start);
			break;
		}

		case stat_return:
			if(t->expr){
				walk_expr(t->expr, t->symtab);
				asm_temp(1, "pop rax");
			}
			asm_temp(1, "jmp %s", curfunc_lblfin);
			break;

		case stat_expr:
			walk_expr(t->expr, t->symtab);
			asm_temp(1, "pop rax ; unused expr");
			break;

		case stat_code:
			if(t->codes){
				tree **iter;

				for(iter = t->codes; *iter; iter++)
					walk_tree(*iter);
			}
			break;

		case stat_noop:
			break;
	}

	asm_nl();
}

void decl_walk_expr(expr *e, symtable *stab)
{
	if(e->type == expr_str)
		/* some arrays will go here too */
		asm_declare_str(e->sym->decl->spel, e->val.s, e->strl);

#define WALK_IF(x) if(x) decl_walk_expr(x, stab)
	WALK_IF(e->lhs);
	WALK_IF(e->expr);
	WALK_IF(e->rhs);
#undef WALK_IF

	if(e->funcargs){
		expr **iter;
		for(iter = e->funcargs; *iter; iter++)
			decl_walk_expr(*iter, stab);
	}
}

void decl_walk_tree(tree *t)
{
#define WALK_IF(x) if(x) decl_walk_tree(x)
	WALK_IF(t->lhs);
	WALK_IF(t->rhs);
#undef WALK_IF

	if(t->codes){
		tree **iter;

		for(iter = t->codes; *iter; iter++)
			decl_walk_tree(*iter);
	}

#define WALK_IF(x) if(x) decl_walk_expr(x, t->symtab)
	WALK_IF(t->expr);

	if(t->flow){
		WALK_IF(t->flow->for_init);
		WALK_IF(t->flow->for_while);
		WALK_IF(t->flow->for_inc);
	}
#undef WALK_IF
}

void gen_asm_func(decl *d)
{
	function *f = d->func;
	if(f->code){
		int offset;

		asm_nl();
		asm_temp(0, "global %s", d->spel);

		/* walk string decls */
		decl_walk_tree(f->code);

		asm_label(d->spel);
		asm_temp(1, "push rbp");
		asm_temp(1, "mov rbp, rsp");

		curfunc_lblfin = asm_code_label(d->spel);

		if((offset = f->code->symtab->auto_offset))
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
	int type_ch;

	if(d->type->spec & spec_extern){
		asm_temp(0, "extern %s", d->spel);
		return;
	}

	if((d->type->spec & spec_static) == 0)
		asm_temp(0, "global %s", d->spel);

	if(d->ptr_depth){
		type_ch = 'q';
	}else{
		switch(d->type->primitive){
			case type_int:
				type_ch = 'q';
				break;

			case type_char:
				type_ch = 'b';
				break;

			case type_void:
				ICE("type primitive is void");

			case type_unknown:
			default:
				ICE("type primitive not set");
		}
	}

	if(d->init){
		/* TODO: direct to .bss */
		/* TODO: arrays */
		switch(d->init->type){
			case expr_val:
				asm_tempf(f, 0, "%s d%c %d", d->spel, type_ch, d->init->val);
				break;

			case expr_addr:
			{
				char type_s = 'b';
				if(d->ptr_depth > 0 || d->type->primitive != type_char)
					type_s = 'q';

				asm_tempf(f, 0, "%s d%c %s", d->spel, type_s, d->init->spel);
				break;
			}

			case expr_str:
				asm_declare_str(d->sym->decl->spel, d->init->val.s, d->init->strl);
				break;

			case expr_cast:
			case expr_sizeof:
			case expr_identifier:
				/* TODO */
				ICE("TODO: init with %s", expr_to_str(d->init->type));
				break;

			default:
				ICE("unexpected global initaliser");
		}

	}else{
		/* TODO: direct to .data */
		int arraylen = 1;
		int i;

		for(i = 0; d->arraysizes && d->arraysizes[i]; i++)
			arraylen = (i + 1) * d->arraysizes[i]->val.i;
		/* TODO: check that i+1 is correct for the order here */

		asm_temp(1, "%s res%c %d", d->spel, type_ch, arraylen);
	}
}

void gen_asm(symtable *globs)
{
	decl **diter;
	for(diter = globs->decls; diter && *diter; diter++){
		decl *d = *diter;

		if(d->ignore)
			continue;

		if(d->func)
			gen_asm_func(d);
		else
			gen_asm_global_var(d);
	}
}
