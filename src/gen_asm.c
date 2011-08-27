#include <stdlib.h>
#include <stdio.h>

#include "tree.h"
#include "macros.h"
#include "asm.h"
#include "platform.h"
#include "sym.h"
#include "asm_op.h"
#include "gen_asm.h"


void walk_expr(expr *e, symtable *tab)
{
	switch(e->type){
		case expr_identifier:
			asm_sym(ASM_LOAD, symtab_search(tab, e->spel), "rax");
			asm_temp("push rax");
			break;

		case expr_val:
			/*asm_new(asm_load_val, &e->val);*/
			asm_temp("mov rax, %d", e->val);
			asm_temp("push rax");
			break;

		case expr_op:
			asm_operate(e, tab);
			break;

		case expr_assign:
			walk_expr(e->expr, tab);
			/*asm_new(asm_assign, e->spel);*/
			asm_temp("pop rax");
			asm_sym(ASM_SET, symtab_search(tab, e->spel), "rax");
			break;

		case expr_funcall:
		{
			expr **iter;
			int nargs = 0;

			if(e->funcargs)
				for(iter = e->funcargs; *iter; iter++){
					walk_expr(*iter, tab);
					nargs++;
				}

			asm_new(asm_call, e->spel);
			if(nargs)
				asm_temp("add rsp, %d ; %d args",
						nargs * platform_word_size(), nargs);
			break;
		}

		case expr_addr:
			asm_sym(ASM_LEA, symtab_search(tab, e->spel), "rax");
			asm_temp("push rax");
			break;

		case expr_sizeof:
			asm_temp("push %d ; sizeof %s", platform_word_size(), e->spel);
			break;

		case expr_str:
			/* the ->spel is the string itself */
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

			asm_label(lbl_for);
			walk_expr(t->flow->for_while, t->symtab);

			asm_temp("pop rax");
			asm_temp("test rax, rax");
			asm_temp("jz %s", lbl_fin);

			walk_tree(t->lhs);
			walk_expr(t->flow->for_inc, t->symtab);

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

		default:
			fprintf(stderr, "walk_tree: TODO %s\n", stat_to_str(t->type));
			break;

		case stat_expr:
			walk_expr(t->expr, t->symtab);
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
		asm_declare_str(e->sym->str_lbl, e->spel);

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
	if(t->type == stat_expr){
		decl_walk_expr(t->expr, t->symtab);
	}else{
		if(t->codes){
			tree **iter;

			for(iter = t->codes; *iter; iter++)
				decl_walk_tree(*iter);
		}

#define WALK_IF(x) if(x) decl_walk_expr(x, t->symtab)
		if(t->flow){
			WALK_IF(t->flow->for_init);
			WALK_IF(t->flow->for_while);
			WALK_IF(t->flow->for_inc);
		}
#undef WALK_IF

#define WALK_IF(x) if(x) decl_walk_tree(x)
		WALK_IF(t->lhs);
		WALK_IF(t->rhs);
#undef WALK_IF
	}
}

void gen_asm(function *f)
{
	if(f->code){
		int offset;
		sym *s;

		asm_temp("global %s", f->func_decl->spel);

		/* walk string + array decl */
		decl_walk_tree(f->code);

		asm_label(f->func_decl->spel);
		asm_temp("push rbp");
		asm_temp("mov rbp, rsp");

		for(s = f->symtab->first; s; s = s->next)
			if(s->type == sym_auto)
				offset += platform_word_size();

		if(offset)
			asm_temp("sub rsp, %d", offset);
		walk_tree(f->code);
		if(offset)
			asm_temp("add rsp, %d", offset);

		asm_temp("leave");
		asm_temp("ret");
	}else{
		asm_temp("extern %s", f->func_decl->spel);
	}
}
