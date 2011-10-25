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
		asm_temp("push rax ; save val");

		walk_expr(store->lhs, stab); /* skip over the *() bit */

		/* move `pop` into `pop` */
		asm_temp("pop rax ; ptr");
		asm_temp("pop rbx ; val");
		asm_temp("mov [rax], rbx");
	}else{
		DIE_ICE();
	}
}

void walk_expr(expr *e, symtable *stab)
{
	switch(e->type){
		case expr_cast:
			/* ignore the lhs, it's just a type spec */
			walk_expr(e->rhs, stab);
			break;

		case expr_if:
		{
			char *lblfin, *lblelse;
			lblfin  = label_code("ifexpa");
			lblelse = label_code("ifexpb");

			walk_expr(e->expr, stab);
			asm_temp("pop rax");
			asm_temp("test rax, rax");
			asm_temp("jz %s", lblelse);
			walk_expr(e->lhs ? e->lhs : e->expr, stab);
			asm_temp("jmp %s", lblfin);
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
			asm_temp("push rax");
			break;

		case expr_val:
			/*asm_new(asm_load_val, &e->val);*/
			asm_temp("mov rax, %d", e->val);
			asm_temp("push rax");
			break;

		case expr_op:
			asm_operate(e, stab);
			break;

		case expr_assign:
			if(e->assign_type == assign_normal){
				walk_expr(e->rhs, stab);
				asm_temp("mov rax, [rsp]");
				asm_ax_to_store(e->lhs, stab);

			}else{
				int flag;

				/*
				 * these aren't actually treated as assignments,
				 * more expressions that alter values
				 */
				walk_expr(e->expr, stab);
				/*
				 * load the identifier and keep it on the stack for returning
				 * now we inc/dec it
				 */

				flag = e->assign_type == assign_pre_increment || e->assign_type == assign_post_increment;

				/* shouldn't need to laod it, but just in case */
				asm_temp("mov rax, [rsp]");
				asm_temp("%s rax", flag ? "inc" : "dec");

				if((flag = e->assign_type == assign_pre_increment) || e->assign_type == assign_pre_decrement)
					/* change the value we are "returning", too */
					asm_temp("mov [rsp], rax");

				/* store back to the sym's home */
				asm_ax_to_store(e->expr, stab);
			}
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
				asm_temp("add rsp, %d ; %d arg%s",
						nargs * platform_word_size(),
						nargs,
						nargs == 1 ? "":"s");

			asm_temp("push rax ; ret");
			break;
		}

		case expr_addr:
			asm_sym(ASM_LEA, e->sym, "rax");
			asm_temp("push rax");
			break;

		case expr_sizeof:
			/* TODO */
			if(e->spel)
				asm_temp("push %d ; sizeof %s", platform_word_size(), e->spel);
			else
				asm_temp("push %d ; sizeof type %s", platform_word_size(), decl_to_str(e->tree_type));
			break;

		case expr_str:
			asm_temp("mov rax, %s", e->sym->str_lbl);
			asm_temp("push rax");
			break;
	}
}

void walk_tree(tree *t)
{
	switch(t->type){
		case stat_if:
		{
			char *lbl_else = label_code("else");
			char *lbl_fi   = label_code("fi");

			walk_expr(t->expr, t->symtab);

			asm_temp("pop rax");
			asm_temp("test rax, rax");
			asm_temp("jz %s", lbl_else);
			walk_tree(t->lhs);
			asm_temp("jmp %s", lbl_fi);
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

			lbl_for = label_code("for");
			lbl_fin = label_code("for_fin");

			walk_expr(t->flow->for_init, t->symtab);
			asm_temp("pop rax ; unused for init");

			asm_label(lbl_for);
			walk_expr(t->flow->for_while, t->symtab);

			asm_temp("pop rax");
			asm_temp("test rax, rax");
			asm_temp("jz %s", lbl_fin);

			walk_tree(t->lhs);
			walk_expr(t->flow->for_inc, t->symtab);
			asm_temp("pop rax ; unused for inc");

			asm_temp("jmp %s", lbl_for);

			asm_label(lbl_fin);

			free(lbl_for);
			free(lbl_fin);
			break;
		}

		case stat_while:
		{
			char *lbl_start, *lbl_fin;

			lbl_start = label_code("while");
			lbl_fin   = label_code("while_fin");

			asm_label(lbl_start);
			walk_expr(t->expr, t->symtab);
			asm_temp("pop rax");
			asm_temp("test rax, rax");
			asm_temp("jz %s", lbl_fin);
			walk_tree(t->lhs);
			asm_temp("jmp %s", lbl_start);
			asm_label(lbl_fin);

			free(lbl_start);
			free(lbl_fin);
			break;
		}

		case stat_do:
		{
			char *lbl_start;

			lbl_start = label_code("do");

			asm_label(lbl_start);
			walk_tree(t->lhs);

			walk_expr(t->expr, t->symtab);
			asm_temp("pop rax");
			asm_temp("test rax, rax");
			asm_temp("jnz %s", lbl_start);

			free(lbl_start);
			break;
		}

		case stat_return:
			walk_expr(t->expr, t->symtab);
			asm_temp("pop rax");
			asm_temp("jmp %s", curfunc_lblfin);
			break;

		default:
			fprintf(stderr, "\x1b[1;31mwalk_tree: TODO %s\x1b[m\n", stat_to_str(t->type));
			break;

		case stat_expr:
			walk_expr(t->expr, t->symtab);
			asm_temp("pop rax ; unused expr");
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
}

void decl_walk_expr(expr *e, symtable *stab)
{
	if(e->type == expr_str)
		/* some arrays will go here too */
		asm_declare_str(e->sym->str_lbl, e->val.s, e->strl);

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

void gen_asm_func(function *f)
{
	if(f->code){
		int offset;

		asm_temp("global %s", f->func_decl->spel);

		/* walk string decls */
		decl_walk_tree(f->code);

		asm_label(f->func_decl->spel);
		asm_temp("push rbp");
		asm_temp("mov rbp, rsp");

		curfunc_lblfin = label_code(f->func_decl->spel);

		if((offset = f->code->symtab->auto_offset))
			asm_temp("sub rsp, %d", offset);
		walk_tree(f->code);
		asm_label(curfunc_lblfin);
		if(offset)
			asm_temp("add rsp, %d", offset);

		asm_temp("leave");
		asm_temp("ret");
		free(curfunc_lblfin);
	}else if(f->func_decl->type->spec & spec_extern){
		asm_temp("extern %s", f->func_decl->spel);
	}
}

void gen_asm_global_var(global *g)
{
	/* MASSIVE TODO */
	if(g->init){
		asm_temp("%s dq %d ; TODO", g->ptr.d->spel, g->init->val);
	}else{
		asm_temp("%s resq 1 ; TODO", g->ptr.d->spel);
	}
}

void gen_asm(global **globs)
{
	for(; *globs; globs++)
		if((*globs)->isfunc)
			gen_asm_func((*globs)->ptr.f);
		else
			gen_asm_global_var(*globs);
}
