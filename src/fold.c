#include <stdio.h>

#include "tree.h"
#include "fold.h"
#include "util.h"
#include "sym.h"
#include "platform.h"


void fold_expr(expr *e, symtable *stab)
{
	switch(e->type){
		case expr_val:
		case expr_addr:
		case expr_sizeof:
			break;

		case expr_assign:
			/* TODO: check e->expr's type is sym_find(e->spel)'s type */
		case expr_identifier:
			if(!symtab_search(stab, e->spel))
				die_at(&e->where, "undeclared identifier \"%s\"", e->spel);
			break;

		case expr_op:
			/* TODO: fold operator */
			break;

		case expr_str:
			/* TODO: escape escapes */
			break;

		case expr_funcall:
			if(e->funcargs){
				expr **iter;
				for(iter = e->funcargs; *iter; iter++)
					fold_expr(*iter, stab);
			}
			break;
	}
}

void fold_code(tree *t, symtable *parent_tab)
{
	switch(t->type){
		case stat_do:
		case stat_if:
		case stat_else:
		case stat_while:
		case stat_for:
		case stat_break:
			fprintf(stderr, "fold TODO with %s\n", stat_to_str(t->type));
			break;

		case stat_code:
			if(t->decls){
				decl **iter;

				if(!parent_tab){
					/* this tree is the first in a function */
					parent_tab = symtab_new();
					t->symtab_parent = 1;
				}

				for(iter = t->decls; *iter; iter++)
					symtab_add(parent_tab, *iter);
			}

			/* decls must be walked first */
			if(t->codes){
				tree **iter;
				for(iter = t->codes; *iter; iter++)
					fold_code(*iter, parent_tab /* init'd */);
			}

			if(parent_tab){
				sym *s;
				int offset = 0;
				for(s = parent_tab->first; s; s = s->next){
					/* extern check goes here */
					s->offset = offset;
					offset += platform_word_size(); /* for now */
				}
			}
			break;

		case stat_expr:
		case stat_return:
			fold_expr(t->expr, parent_tab);
			break;

		case stat_noop:
			break;
	}

	t->symtab = parent_tab;
}

void fold_decl(decl *d)
{
	if(d->type == type_void && !d->ptr_depth)
		die_at(&d->where, "can't have a void variable");
}

void fold_func(function *f)
{
	decl **diter;

	fold_decl(f->func_decl);

	for(diter = f->args; diter && *diter; diter++)
		fold_decl(*diter);

	if(f->code)
		fold_code(f->code, NULL);
}

void fold(function **fs)
{
	function **f;

	for(f = fs; *f; f++)
		fold_func(*f /* TODO: (*f)->parent_symtable = global_syms */);
}
