#include <stdio.h>
#include <string.h>

#include "tree.h"
#include "fold.h"
#include "util.h"
#include "sym.h"
#include "platform.h"
#include "asm.h"
#include "const.h"

#define DIE_UNDECL() \
		die_at(&e->where, "undeclared identifier \"%s\" (%s:%d)", e->spel, __FILE__, __LINE__)

int fold_is_lvalue(expr *e)
{
	/*
	 * valid lvaluess:
	 *
	 *   x = 5;
	 *   *(expr) = 5;
	 *
	 *
	 * also can't be const
	 */

	return e->type == expr_identifier ||
		(e->type == expr_op && e->op == op_deref);
}

void fold_expr(expr *e, symtable *stab)
{
#define GET_VARTYPE(from) \
	do{ \
		memcpy(&e->vartype, from, sizeof e->vartype); \
		e->vartype.spel = NULL; \
		e->vartype.arraysizes = NULL; \
	}while(0)

	if(e->spel)
		e->sym = symtab_search(stab, e->spel);

	const_fold(e);

	switch(e->type){
		case expr_val:
		case expr_sizeof:
			e->vartype.type = type_int;
			break;

		case expr_addr:
			if(!e->sym)
				DIE_UNDECL();

			GET_VARTYPE(e->sym->decl);
			e->vartype.ptr_depth++;
			break;

		case expr_identifier:
			if(!e->sym)
				DIE_UNDECL();

			GET_VARTYPE(e->sym->decl);
			break;

		case expr_assign:
			if(!fold_is_lvalue(e->lhs))
				die_at(&e->lhs->where, "not an lvalue");

			fold_expr(e->lhs, stab);
			fold_expr(e->rhs, stab);

			if(e->sym)
				/* read the vartype from what we're assigning to, not the expr */
				GET_VARTYPE(e->sym->decl);
			else
				/* get the vartype from the dereference's vartype */
				GET_VARTYPE(&e->lhs->vartype);
			break;


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
					die_at(&e->where, "can't dereference non-pointer (%s)", type_to_str(e->vartype.type));
			}
			break;

		case expr_str:
		{
			sym *sym;
			sym = symtab_add(stab, decl_new(), sym_str);

			sym->str_lbl = label_str();

			/* e->sym shouldn't be !NULL anyway */
			e->sym = sym;

			e->vartype.type      = type_char;
			e->vartype.ptr_depth = 1;
			break;
		}

		case expr_funcall:
			if(e->funcargs){
				expr **iter;
				for(iter = e->funcargs; *iter; iter++)
					fold_expr(*iter, stab);
			}

			if(e->sym){
				GET_VARTYPE(e->sym->decl); /* XXX: check */
			}else{
				fprintf(stderr, "warning: %s undeclared, assuming return type int\n", e->spel);
				e->vartype.type = type_int;
			}
			break;
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
		case stat_do:
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

				auto_offset = arg_offset = 0; /* FIXME? { int i; { int j; } } */

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

void fold_decl(decl *d, symtable *stab)
{
	int i;

	if(d->type == type_void && !d->ptr_depth && !d->func)
		die_at(&d->where, "can't have a void variable");

	for(i = 0; d->arraysizes && d->arraysizes[i]; i++){
		fold_expr(d->arraysizes[i], stab);

		if(!fold_expr_is_const(d->arraysizes[i]))
			die_at(&d->arraysizes[i]->where, "not a constant expression");
	}
}

void fold_func(function *f)
{
	decl **diter;

	fold_decl(f->func_decl, NULL /* TODO: global symtab */);

	for(diter = f->args; diter && *diter; diter++)
		fold_decl(*diter, NULL /* TODO: also here */);

	if(f->code){
		decl **d;
		sym *s;

		f->symtab = symtab_new();

		if(f->args)
			/* check for unnamed params */
			for(d = f->args; *d; d++)
				if(!(*d)->spel)
					die_at(&f->where, "function \"%s\" has unnamed arguments", f->func_decl->spel);
				else
					symtab_add(f->symtab, *d, sym_arg);


		fold_code(f->code, f->symtab);

		for(s = f->symtab->first; s; s = s->next)
			fold_decl(s->decl, f->symtab);
	}
}

void fold(function **fs)
{
	function **f;

	for(f = fs; *f; f++)
		fold_func(*f /* TODO: (*f)->parent_symtable = global_syms */);
}
