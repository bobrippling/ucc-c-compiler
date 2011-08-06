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
	t->symtab = parent_tab;

	switch(t->type){
		case stat_do:
		case stat_while:
		case stat_for:
		case stat_break:
			fprintf(stderr, "fold TODO with %s\n", stat_to_str(t->type));
			break;

		case stat_if:
			fold_expr(t->expr, parent_tab);
			fold_code(t->lhs,  parent_tab);
			break;

		case stat_code:
			if(t->decls){
				decl **iter;

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
				int auto_offset, arg_offset;
				sym *s;

				auto_offset = arg_offset = 0;

				for(s = parent_tab->first; s; s = s->next){
					/* extern check goes here */
					if(s->type == sym_auto){
						s->offset = auto_offset;
						auto_offset += platform_word_size();
					}else{
						s->offset = arg_offset;
						arg_offset += platform_word_size();
					}
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
}

void fold_decl(decl *d)
{
	if(d->type == type_void && !d->ptr_depth && !d->func)
		die_at(&d->where, "can't have a void variable");
}

void fold_func(function *f)
{
	decl **diter;

	fold_decl(f->func_decl);

	for(diter = f->args; diter && *diter; diter++)
		fold_decl(*diter);

	if(f->code){
		decl **d;

		f->symtab = symtab_new();

		if(f->args)
			for(d = f->args; *d; d++){
				sym *new = symtab_add(f->symtab, *d);
				new->type = sym_arg;
			}

		fold_code(f->code, f->symtab);
	}
}

void fold(function **fs)
{
	function **f;

	for(f = fs; *f; f++)
		fold_func(*f /* TODO: (*f)->parent_symtable = global_syms */);
}
