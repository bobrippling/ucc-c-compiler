#include <stdlib.h>
#include <stdio.h>

#include "tree.h"
#include "macros.h"
#include "asm.h"
#include "platform.h"
#include "sym.h"
#include "asm_op.h"
#include "gen_asm.h"

#define WALK_IF(st, sub, fn) \
	if(st->sub) \
		fn(st->sub)

void walk_expr(expr *e, symtable *tab)
{
	switch(e->type){
		case expr_identifier:
			/*asm_new(asm_load_ident, e->spel);*/
			asm_temp("mov rax, [rbp - %d]", symtab_search(tab, e->spel)->offset);
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
			asm_temp("mov [rbp - %d], rax", symtab_search(tab, e->spel)->offset);
			break;

		case expr_funcall:
		{
			expr **iter;

			if(e->funcargs)
				for(iter = e->funcargs; *iter; iter++)
					walk_expr(*iter, tab);

			asm_new(asm_call, e->spel);
			asm_temp("; TODO: pop args");
			break;
		}

		case expr_addr:
			asm_new(asm_addrof, e->spel);
			break;

		case expr_sizeof:
			fprintf(stderr, "TODO: sizeof\n");
			break;

		case expr_str:
			fprintf(stderr, "TODO: walk_expr with \"%s\"\n", e->spel);
			break;
	}
}

void walk_tree(tree *t)
{
	int offset = 0;

	WALK_IF(t, lhs, walk_tree);
	WALK_IF(t, rhs, walk_tree);
	if(t->expr)
		walk_expr(t->expr, t->symtab);

	if(t->symtab_parent){
		sym *s;

		for(s = t->symtab->first; s; s = s->next)
			offset += s->offset;
		asm_temp("sub rsp, %d", offset);
	}

	if(t->codes){
		tree **iter;

		for(iter = t->codes; *iter; iter++)
			walk_tree(*iter);
	}

	if(offset)
		asm_temp("add rsp, %d", offset);
}

void walk_fn(function *f)
{
	if(f->code){
		asm_temp("%s:", f->func_decl->spel);
		asm_temp("push rbp");
		asm_temp("mov rbp, rsp");
		walk_tree(f->code);
		asm_temp("leave");
		asm_temp("ret");
	}
}
