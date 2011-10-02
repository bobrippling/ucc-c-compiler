#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "tree.h"
#include "fold.h"
#include "util.h"
#include "sym.h"
#include "platform.h"
#include "asm.h"
#include "const.h"

#define DIE_UNDECL() \
		die_at(&e->where, "undeclared identifier \"%s\" (%s:%d)", e->spel, __FILE__, __LINE__)

static global **fold_globals;

int fold_is_lvalue(expr *e)
{
	/*
	 * valid lvaluess:
	 *
	 *   x = 5;
	 *   *(expr) = 5;
	 *   (cast *)expr = 5;
	 *
	 * also can't be const
	 */
	if(e->vartype->spec & spec_const)
		die_at(&e->where, "can't modify const expression");

	return
		 e->type == expr_identifier ||
		(e->type == expr_op && e->op == op_deref && fold_is_lvalue(e->lhs)) ||
		(e->type == expr_cast && e->lhs->vartype->ptr_depth) /* assignment to pointer-deref */
		;
}

void fold_expr(expr *e, symtable *stab)
{
#define GET_VARTYPE(from) \
	do{ \
		if(e->vartype) \
			type_free(e->vartype); \
		e->vartype = type_copy(from); \
	}while(0)

	if(e->spel)
		e->sym = symtab_search(stab, e->spel, fold_globals);

	const_fold(e);

	switch(e->type){
		case expr_val:
		case expr_sizeof:
			e->vartype->primitive = type_int;
			break;

		case expr_if:
			fold_expr(e->expr, stab);
			if(e->lhs)
				fold_expr(e->lhs,  stab);
			fold_expr(e->rhs,  stab);
			GET_VARTYPE(e->rhs->vartype); /* TODO: check they're the same */
			break;

		case expr_cast:
			fold_expr(e->rhs, stab);
			GET_VARTYPE(e->lhs->vartype);
			break;

		case expr_addr:
			if(!e->sym)
				DIE_UNDECL();

			GET_VARTYPE(e->sym->decl->type);
			e->vartype->ptr_depth++;
			break;

		case expr_identifier:
			if(!e->sym)
				DIE_UNDECL();

			GET_VARTYPE(e->sym->decl->type);
			break;

		case expr_assign:
		{
			const int assign   = e->assign_type == assign_normal;
			expr *const use_me = assign ? e->lhs : e->expr;

			if(!fold_is_lvalue(use_me))
				die_at(&use_me->where, "not an lvalue");

			if(e->assign_type == assign_normal){
				fold_expr(e->lhs, stab);
				fold_expr(e->rhs, stab);
			}else{
				fold_expr(e->expr, stab);
			}

			if(e->sym)
				/* read the vartype from what we're assigning to, not the expr */
				GET_VARTYPE(e->sym->decl->type);
			else
				/* get the vartype from the dereference's vartype */
				GET_VARTYPE(use_me->vartype);


			if(!assign && e->vartype->ptr_depth && (e->vartype->ptr_depth > 1 || e->vartype->primitive != type_char)){
				/*
				 * we're inc/dec'ing a pointer, we need to inc by sizeof(*ptr)
				 * convert from inc/dec to a standard addition
				 *
				 * if-optimisation above - don't change inc to + if it's a char *
				 */
				expr *addition = expr_new();

				type_free(addition->vartype);
				addition->vartype = type_copy(e->vartype);

				addition->type = expr_op;
				switch(e->assign_type){
					case assign_pre_increment:
					case assign_post_increment:
						addition->op = op_plus;
						break;
					case assign_pre_decrement:
					case assign_post_decrement:
						addition->op = op_minus;
						break;
					case assign_normal:
						DIE_ICE();
				}

				addition->lhs = e->expr;
				addition->rhs = expr_ptr_multiply(expr_new_val(1), addition->vartype);

				e->assign_type = assign_normal;
				e->lhs         = e->expr;
				e->rhs         = addition;
			}

			break;
		}


		case expr_op:
			fold_expr(e->lhs, stab);
			if(e->rhs)
				fold_expr(e->rhs, stab);

			/* XXX: note, this assumes that e.g. "1 + 2" the lhs and rhs have the same type */
			if(e->op == op_deref){
				GET_VARTYPE(e->lhs->vartype);

				e->vartype->ptr_depth--;

				if(e->vartype->ptr_depth == 0)
					switch(e->lhs->vartype->primitive){
						case type_unknown:
						case type_void:
							die_at(&e->where, "can't dereference void pointer");
						default:
							/* e->vartype already set to deref type */
							break;
					}
				else if(e->vartype->ptr_depth < 0)
					die_at(&e->where, "can't dereference non-pointer (%s)", type_to_str(e->vartype));
			}else{
				/* look either side - if either is a pointer, take that as the vartype */
				/* TODO: checks for pointer + pointer, etc etc */
				if(e->rhs && e->rhs->vartype->ptr_depth)
					GET_VARTYPE(e->rhs->vartype);
				else
					GET_VARTYPE(e->lhs->vartype);

				switch(e->op){
					case op_plus:
					case op_minus:
						if(e->vartype->ptr_depth && e->rhs){
							/* we're dealing with pointers, adjust the amount we add by */

							if(e->lhs->vartype->ptr_depth)
								/* lhs is the pointer, we're adding on rhs, hence multiply rhs by lhs's ptr size */
								e->rhs = expr_ptr_multiply(e->rhs, e->lhs->vartype);
							else
								e->lhs = expr_ptr_multiply(e->lhs, e->rhs->vartype);
						}
					default:
						break;
				}
			}
			break;

		case expr_str:
		{
			sym *sym;
			sym = symtab_add(stab, decl_new(), sym_str);

			sym->str_lbl = label_str();

			/* e->sym shouldn't be !NULL anyway */
			e->sym = sym;

			e->vartype->primitive = type_char;
			e->vartype->ptr_depth = 1;
			break;
		}

		case expr_funcall:
			if(e->funcargs){
				expr **iter;
				for(iter = e->funcargs; *iter; iter++)
					fold_expr(*iter, stab);
			}

			if(!e->sym){
				symtable *paren;
				function *f;

				f = function_new();
				f->func_decl = decl_new();

				f->func_decl->type->primitive = type_int;
				f->func_decl->type->func = 1;
				f->func_decl->spel = e->spel;

				memcpy(&f->func_decl->where,       &e->where, sizeof e->where);
				memcpy(&f->func_decl->type->where, &e->where, sizeof e->where);

				fprintf(stderr, "%s: function \"%s\" undeclared, assuming return type int\n", where_str(&e->where), e->spel);

				dynarray_add((void ***)&fold_globals, global_new(f, NULL));

				for(paren = stab; paren->parent; paren = paren->parent);
				e->sym = symtab_add(paren, f->func_decl, sym_func);
			}

			GET_VARTYPE(e->sym->decl->type); /* XXX: check */
			break;
	}
#undef GET_VARTYPE
}

void fold_type(type *t)
{
	if(t->primitive == type_void && !t->ptr_depth && !t->func)
		die_at(&t->where, "can't have a void variable");
}

void fold_decl(decl *d, symtable *stab)
{
	int i;

	fold_type(d->type);

	for(i = 0; d->arraysizes && d->arraysizes[i]; i++){
		fold_expr(d->arraysizes[i], stab);

		if(!fold_expr_is_const(d->arraysizes[i]))
			die_at(&d->arraysizes[i]->where, "not a constant expression");
	}
}

void fold_code(tree *t)
{
	switch(t->type){
		default:
			fprintf(stderr, "fold TODO with %s\n", stat_to_str(t->type));
			break;

		case stat_while:
		case stat_do:
		case stat_if:
			fold_expr(t->expr, t->symtab);

			symtab_nest(t->symtab, &t->lhs->symtab);
			fold_code(t->lhs);

			if(t->rhs){
				symtab_nest(t->symtab, &t->rhs->symtab);
				fold_code(t->rhs);
			}
			break;

		case stat_for:
			fold_expr(t->flow->for_init,  t->symtab);
			fold_expr(t->flow->for_while, t->symtab);
			fold_expr(t->flow->for_inc,   t->symtab);

			symtab_nest(t->symtab, &t->lhs->symtab);
			fold_code(t->lhs);
			break;

		case stat_code:
		{
			int auto_offset, arg_offset;
			sym *s;

			if(t->decls){
				decl **iter;

				for(iter = t->decls; *iter; iter++){
					symtab_add(t->symtab, *iter, sym_auto);
					fold_decl(*iter, t->symtab);
				}
			}


			auto_offset = t->symtab->parent ? t->symtab->parent->auto_offset : 0;
			arg_offset  = 0;

			for(s = t->symtab->first; s; s = s->next)
				if(s->type == sym_auto){
					s->offset = auto_offset;

					/* TODO: optimise for chars / don't assume everything is an int */
					if(s->decl->arraysizes){
						/* should've been folded fully */
						int i;
						for(i = 0; s->decl->arraysizes[i]; i++)
							auto_offset += s->decl->arraysizes[i]->val.i * platform_word_size();

						auto_offset += platform_word_size();
					}else{
						/* assume sizeof(int) for chars etc etc */
						auto_offset += platform_word_size();
					}
				}else if(s->type == sym_arg){
					s->offset = arg_offset;
					arg_offset += platform_word_size();
				}

			t->symtab->auto_offset = auto_offset;

			if(t->codes){
				tree **iter;
				int subtab_offsets = 0;

				for(iter = t->codes; *iter; iter++){
					int offset;

					symtab_nest(t->symtab, &(*iter)->symtab);
					fold_code(*iter);
					offset = (*iter)->symtab->auto_offset;

					if(offset > subtab_offsets)
						subtab_offsets = offset;
						/*
						 * we only need take the largest, other space can be reused
						 * because:
						 * {
						 *   int i;
						 * }
						 * {
						 *   int j;
						 * }
						 * never needs to access i and j at the same time
						 */
				}

				t->symtab->auto_offset += subtab_offsets;
			}
			break;
		}

		case stat_expr:
		case stat_return:
			fold_expr(t->expr, t->symtab);
			break;

		case stat_noop:
			break;
	}
}

void fold_func(function *f, symtable *globsyms, global **globs)
{
	decl **diter;

	fold_decl(f->func_decl, globsyms);

	for(diter = f->args; diter && *diter; diter++)
		fold_decl(*diter, globsyms);

	if(f->code){
		if(f->args){
			/* check for unnamed params */
			int nargs, i;

			for(nargs = 0; f->args[nargs]; nargs++);

			/* add args backwards, since we push them onto the stack backwards */
			for(i = nargs - 1; i >= 0; i--)
				if(!f->args[i]->spel)
					die_at(&f->where, "function \"%s\" has unnamed arguments", f->func_decl->spel);
				else
					symtab_add(f->code->symtab, f->args[i], sym_arg);
		}

		symtab_nest(globsyms, &f->code->symtab);

		fold_code(f->code);
	}else{
		global **iter;
		int found = 0;

		for(iter = globs; *iter; iter++)
			if((*iter)->isfunc){
				function *f2 = (*iter)->ptr.f;

				if(f2->code && !strcmp(f2->func_decl->spel, f->func_decl->spel)){
					found = 1;
					break;
				}
			}

		if(!found){
			f->func_decl->type->spec |= spec_extern;
			fprintf(stderr, "%s: assuming \"%s\" is extern\n", where_str(&f->where), f->func_decl->spel);
		}
	}
}

void fold(global ***globs)
{
	symtable *stab = symtab_new();
	int i;

	fold_globals = *globs;

	/* globs is now invalid, since it might change, thus we must use fold_globals */
	for(i = 0; fold_globals[i]; i++){
		if(fold_globals[i]->isfunc){
			symtab_add(stab, fold_globals[i]->ptr.f->func_decl, sym_func);
			fold_func(fold_globals[i]->ptr.f, stab, fold_globals);
		}else{
			symtab_add(stab, fold_globals[i]->ptr.d, sym_auto);
			fold_decl(fold_globals[i]->ptr.d, stab);
		}
	}

	*globs = fold_globals;

	/* FIXME: free symtab if we only have decls */
}
