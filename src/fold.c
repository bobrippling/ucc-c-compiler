#include <stdio.h>

#include "tree.h"
#include "fold.h"
#include "util.h"
#include "sym.h"
#include "platform.h"
#include "asm.h"


typedef struct
{
	enum type deref_type;
	int deref_depth;
} fold_state;

void fold_expr(expr *e, symtable *stab, fold_state *fs)
{
	switch(e->type){
		case expr_val:
		case expr_addr:
		case expr_sizeof:
			break;

		case expr_identifier:
		case expr_assign:
			/* TODO: check e->expr's type is sym_find(e->spel)'s type */
			if(!symtab_search(stab, e->spel))
				die_at(&e->where, "undeclared identifier \"%s\"", e->spel);

			if(e->type == expr_assign){
				fold_expr(e->expr, stab, fs);
			}else if(fs->deref_depth && fs->deref_type == type_unknown){
				/*
					* we've found the symbol we're trying to deref
					* int x = d->ptr_depth - deref_depth
					*
					* if x == 0, we deref by a size of e->spel's type
					* if x >  0, we are doing something like:
					*            char ****x; *x;
					*            hence dereferencing to a pointer-size
					* if x <  0, we are doing something like
					*            char *x; ***x;
					*/
				decl *d;
				int diff;

				d = symtab_search(stab, e->spel)->decl;
				diff = d->ptr_depth - fs->deref_depth;

				if(diff == 0)
					fs->deref_type = d->type;
				else if(diff > 0)
					fs->deref_type = type_ptr;
				else /*if(diff < 0)*/
					die_at(&e->where, "dereferencing a non-pointer");
			}
			break;


		case expr_op:
		{
			fold_state newfs;

			if(e->op == op_deref){
				/* descend, searching for a variable to base the deref on */
				newfs.deref_type  = type_unknown;
				newfs.deref_depth = fs->deref_depth + 1;
			}else{
				newfs = *fs;
			}

#define WALK_IF(x) if(x) fold_expr(x, stab, &newfs)
			WALK_IF(e->lhs);
			WALK_IF(e->expr);
			WALK_IF(e->rhs);
#undef WALK_IF

			if(e->op == op_deref){
				e->deref_type = newfs.deref_type;
				if(e->deref_type == type_unknown)
					die_at(&e->where, "no identifier to dereference");

				if(fs->deref_depth){
					/* we're in a dereference sub-tree */
					fs->deref_type = type_ptr; /* obviously pointer-to-pointer */
				}else{
					/* we are the top of the deref sub-tree */
					fs->deref_type  = newfs.deref_type;
				}
			}else{
				*fs = newfs;
			}
			break;
		}

		case expr_str:
		{
			/* TODO: escape escapes? */
			/* TODO: pre-walk for all strings, add a _name db "str..." */
			/* TODO: string id, for lookups later on */
			sym *sym;
			sym = symtab_add(stab, decl_new(), sym_str);

			sym->str_lbl = label_str();

			e->sym = sym;
			break;
		}

		case expr_funcall:
			if(e->funcargs){
				expr **iter;
				for(iter = e->funcargs; *iter; iter++)
					fold_expr(*iter, stab, fs);
			}
			break;
	}
}

void fold_code(tree *t, symtable *parent_tab, fold_state *fs)
{
	t->symtab = parent_tab;

	switch(t->type){
		default:
			fprintf(stderr, "fold TODO with %s\n", stat_to_str(t->type));
			break;

		case stat_while:
		case stat_if:
			fold_expr(t->expr, parent_tab, fs);
			fold_code(t->lhs,  parent_tab, fs);
			if(t->rhs)
				fold_code(t->rhs,  parent_tab, fs);
			break;

		case stat_for:
			fold_expr(t->flow->for_init,  parent_tab, fs);
			fold_expr(t->flow->for_while, parent_tab, fs);
			fold_expr(t->flow->for_inc,   parent_tab, fs);
			fold_code(t->lhs,             parent_tab, fs);
			break;

		case stat_code:
			if(t->decls){
				decl **iter;

				for(iter = t->decls; *iter; iter++)
					symtab_add(parent_tab, *iter, sym_auto);
			}

			/* decls must be walked first */
			if(t->codes){
				tree **iter;
				for(iter = t->codes; *iter; iter++)
					fold_code(*iter, parent_tab /* init'd */, fs);
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
			fold_expr(t->expr, parent_tab, fs);
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
	fold_state fs;

	fs.deref_type  = type_unknown;
	fs.deref_depth = 0;

	fold_decl(f->func_decl);

	for(diter = f->args; diter && *diter; diter++)
		fold_decl(*diter);

	if(f->code){
		decl **d;

		if(f->args){
			decl **iter;
			/* check for unnamed params */
			for(iter = f->args; *iter; iter++)
				if(!(*iter)->spel)
					die_at(&f->where, "function \"%s\" has unnamed arguments", f->func_decl->spel);
		}

		f->symtab = symtab_new();

		if(f->args)
			for(d = f->args; *d; d++)
				symtab_add(f->symtab, *d, sym_arg);

		fold_code(f->code, f->symtab, &fs);
	}
}

void fold(function **fs)
{
	function **f;

	for(f = fs; *f; f++)
		fold_func(*f /* TODO: (*f)->parent_symtable = global_syms */);
}
