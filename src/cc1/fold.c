#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "fold.h"
#include "../util/util.h"
#include "sym.h"
#include "../util/platform.h"
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
	if(e->tree_type->type->spec & spec_const)
		die_at(&e->where, "can't modify const expression");

	return
		 e->type == expr_identifier ||
		(e->type == expr_op && e->op == op_deref && fold_is_lvalue(e->lhs)) ||
		(e->type == expr_cast && e->lhs->tree_type->ptr_depth) /* assignment to pointer-deref */
		;
}

void fold_expr(expr *e, symtable *stab)
{
#define GET_TREE_TYPE(from) \
	do{ \
		if(e->tree_type) \
			decl_free(e->tree_type); \
		e->tree_type = decl_copy(from); \
	}while(0)

	if(e->spel)
		e->sym = symtab_search(stab, e->spel, fold_globals);

	const_fold(e);

	switch(e->type){
		case expr_val:
		case expr_sizeof:
			e->tree_type->type->primitive = type_int;
			break;

		case expr_if:
			fold_expr(e->expr, stab);
			if(e->lhs)
				fold_expr(e->lhs,  stab);
			fold_expr(e->rhs,  stab);
			GET_TREE_TYPE(e->rhs->tree_type); /* TODO: check they're the same */
			break;

		case expr_cast:
			fold_expr(e->rhs, stab);
			GET_TREE_TYPE(e->lhs->tree_type);
			break;

		case expr_addr:
			if(!e->sym)
				DIE_UNDECL();

			GET_TREE_TYPE(e->sym->decl);
			e->tree_type->ptr_depth++;
			break;

		case expr_identifier:
			if(!e->sym)
				DIE_UNDECL();

			GET_TREE_TYPE(e->sym->decl);
			break;

		case expr_assign:
		{
			int assign;
			expr *use_me;

			if(e->assign_type == assign_augmented){
				/* make a normal op from it */
				expr *rhs = expr_new();

				rhs->type = expr_op;
				rhs->op   = e->op;
				rhs->lhs  = e->lhs;
				rhs->rhs  = e->rhs;

				e->assign_type = assign_normal;
				e->rhs = rhs;
			}

			assign = e->assign_type == assign_normal;
			use_me = assign ? e->lhs : e->expr;

			if(!fold_is_lvalue(use_me))
				die_at(&use_me->where, "not an lvalue");

			if(assign){
				fold_expr(e->lhs, stab);
				fold_expr(e->rhs, stab);
			}else{
				fold_expr(e->expr, stab);
			}

			if(e->sym)
				/* read the tree_type from what we're assigning to, not the expr */
				GET_TREE_TYPE(e->sym->decl);
			else
				/* get the tree_type from the dereference's tree_type */
				GET_TREE_TYPE(use_me->tree_type);


			if(!assign && e->tree_type->ptr_depth && (e->tree_type->ptr_depth > 1 || e->tree_type->type->primitive != type_char)){
				/*
				 * we're inc/dec'ing a pointer, we need to inc by sizeof(*ptr)
				 * convert from inc/dec to a standard addition
				 *
				 * if-optimisation above - don't change inc to + if it's a char *
				 */
				expr *addition = expr_new();

				type_free(addition->tree_type);
				addition->tree_type = decl_copy(e->tree_type);

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
					case assign_augmented:
					case assign_normal:
						DIE_ICE();
				}

				addition->lhs = e->expr;
				addition->rhs = expr_ptr_multiply(expr_new_val(1), addition->tree_type);

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
				GET_TREE_TYPE(e->lhs->tree_type);

				e->tree_type->ptr_depth--;

				if(e->tree_type->ptr_depth == 0)
					switch(e->lhs->tree_type->type->primitive){
						case type_unknown:
						case type_void:
							die_at(&e->where, "can't dereference void pointer");
						default:
							/* e->tree_type already set to deref type */
							break;
					}
				else if(e->tree_type->ptr_depth < 0)
					die_at(&e->where, "can't dereference non-pointer (%s)", type_to_str(e->tree_type->type));
			}else{
				/* look either side - if either is a pointer, take that as the tree_type */
				/* TODO: checks for pointer + pointer, etc etc */
				if(e->rhs && e->rhs->tree_type->ptr_depth)
					GET_TREE_TYPE(e->rhs->tree_type);
				else
					GET_TREE_TYPE(e->lhs->tree_type);

				switch(e->op){
					case op_plus:
					case op_minus:
						if(e->tree_type->ptr_depth && e->rhs){
							/* we're dealing with pointers, adjust the amount we add by */

							if(e->lhs->tree_type->ptr_depth)
								/* lhs is the pointer, we're adding on rhs, hence multiply rhs by lhs's ptr size */
								e->rhs = expr_ptr_multiply(e->rhs, e->lhs->tree_type);
							else
								e->lhs = expr_ptr_multiply(e->lhs, e->rhs->tree_type);
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

			e->tree_type->type->primitive = type_char;
			e->tree_type->ptr_depth = 1;
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

			GET_TREE_TYPE(e->sym->decl); /* XXX: check */
			break;
	}
#undef GET_TREE_TYPE
}

void fold_decl(decl *d, symtable *stab)
{
	int i;

	if(d->type->primitive == type_void && !d->ptr_depth && !d->type->func)
		die_at(&d->type->where, "can't have a void variable");


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

			if(fold_globals[i]->init){
				fold_expr(fold_globals[i]->init,  stab);
				if(const_fold(fold_globals[i]->init))
					/* yes I know fold_expr does const_fold, but this is a decent way to check */
					die_at(&fold_globals[i]->init->where, "not a constant expression");
			}
		}
	}

	*globs = fold_globals;

	/* FIXME: free symtab if we only have decls */
}
