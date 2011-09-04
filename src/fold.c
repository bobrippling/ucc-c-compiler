#include <stdio.h>
#include <string.h>

#include "tree.h"
#include "fold.h"
#include "util.h"
#include "sym.h"
#include "platform.h"
#include "asm.h"

#define DIE_UNDECL() \
		die_at(&e->where, "undeclared identifier \"%s\"", e->spel)

void fold_expr(expr *e, symtable *stab)
{
#define GET_VARTYPE(from) \
	do{ \
		memcpy(&e->vartype, from, sizeof e->vartype); \
		e->vartype.spel = NULL; \
	}while(0)

	switch(e->type){
		case expr_val:
		case expr_sizeof:
			e->vartype.type = type_int;
			break;

		case expr_addr:
		{
			sym *s = symtab_search(stab, e->spel);
			if(!s)
				DIE_UNDECL();

			e->vartype.type = s->decl->type;
			e->vartype.ptr_depth = 1;
			break;
		}

		case expr_identifier:
		{
			sym *s;

			s = symtab_search(stab, e->spel);
			if(!s)
				DIE_UNDECL();

			GET_VARTYPE(s->decl);
			break;
		}

		case expr_assign:
		{
			sym *s;
			if(!(s = symtab_search(stab, e->spel)))
				DIE_UNDECL();

			fold_expr(e->expr, stab);
			/* read the vartype from what we're assigning to, not the expr */
			GET_VARTYPE(s->decl);
			break;
		}


		case expr_op:
			fold_expr(e->lhs, stab);
			if(e->rhs)
				fold_expr(e->rhs, stab);

			/* XXX: note, this assumes that e.g. "1 + 2" the lhs and rhs have the same type */
			GET_VARTYPE(&e->lhs->vartype);

			if(e->op == op_deref){
				e->vartype.ptr_depth--;

				if(e->vartype.ptr_depth == 0)
					switch(e->lhs->vartype.type){
						case type_unknown:
						case type_void:
							die_at(&e->where, "can't dereference void pointer");
						default:
							/* e->vartype.type already set to deref type */
							break;
					}
				else if(e->vartype.ptr_depth < 0)
					die_at(&e->where, "can't dereference non-pointer");
			}
			break;

		case expr_str:
		{
			sym *sym;
			sym = symtab_add(stab, decl_new(), sym_str);

			sym->str_lbl = label_str();

			e->sym = sym;

			e->vartype.type      = type_char;
			e->vartype.ptr_depth = 1;
			break;
		}

		case expr_funcall:
		{
			sym *s;

			if(e->funcargs){
				expr **iter;
				for(iter = e->funcargs; *iter; iter++)
					fold_expr(*iter, stab);
			}

			s = symtab_search(stab, e->spel);
			if(s){
				GET_VARTYPE(s->decl); /* XXX: check */
			}else{
				fprintf(stderr, "warning: %s undeclared, assuming return type int\n", e->spel);
				e->vartype.type = type_int;
			}
			break;
		}
	}
#undef GET_VARTYPE
}

void fold_code(tree *t, symtable *parent_tab)
{
	t->symtab = parent_tab;

	switch(t->type){
		default:
			fprintf(stderr, "fold TODO with %s\n", stat_to_str(t->type));
			break;

		case stat_while:
		case stat_if:
			fold_expr(t->expr, parent_tab);
			fold_code(t->lhs,  parent_tab);
			if(t->rhs)
				fold_code(t->rhs,  parent_tab);
			break;

		case stat_for:
			fold_expr(t->flow->for_init,  parent_tab);
			fold_expr(t->flow->for_while, parent_tab);
			fold_expr(t->flow->for_inc,   parent_tab);
			fold_code(t->lhs,             parent_tab);
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
					fold_code(*iter, parent_tab /* init'd */);
			}

			if(parent_tab){
				int auto_offset, arg_offset;
				sym *s;

				auto_offset = arg_offset = 0;

				for(s = parent_tab->first; s; s = s->next){
					/* extern check goes here */
					switch(s->type){
						case sym_auto:
							s->offset = auto_offset;
							auto_offset += platform_word_size();
							break;

						case sym_arg:
							s->offset = arg_offset;
							arg_offset += platform_word_size();
							break;

						case sym_str:
							break;
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

		fold_code(f->code, f->symtab);
	}
}

void fold(function **fs)
{
	function **f;

	for(f = fs; *f; f++)
		fold_func(*f /* TODO: (*f)->parent_symtable = global_syms */);
}
