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

	if(e->spel){
		if(!e->sym)
			e->sym = symtab_search(stab, e->spel);
		else
			warn_at(&e->where, "ICW: expression for \"%s\" already has a symbol\n", e->spel);
	}

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
				decl *d = decl_new();;

				d->func = function_new();

				d->type->primitive = type_int;
				d->spel = e->spel;

				memcpy(&d->where,       &e->where, sizeof e->where);
				memcpy(&d->type->where, &e->where, sizeof e->where);

				warn_at(&e->where, "function \"%s\" undeclared, assuming return type int", e->spel);

				e->sym = symtab_add(symtab_grandparent(stab), d, sym_func);
			}

			GET_TREE_TYPE(e->sym->decl); /* XXX: check */
			break;
	}
#undef GET_TREE_TYPE
}

void fold_decl(decl *d, symtable *stab)
{
	int i;

#if 0
	/* if global extern... */
	if(!stab->parent && d->type->type_spec & spec_extern){
		/* check all other globs for an instantiation of this decl */
		decl **iter;
		for(iter = stab->decls; iter && *iter; iter++){
			decl *dit = *iter;

			if(dit->spel && !strcmp(dit->spel, d->spel)){
				if(!(dit->type->type_spec & spec_extern))
					;
			}
		}
	}
#endif

	if(d->init){
		fold_expr(d->init, stab);
		if(const_fold(d->init))
			/* yes I know fold_expr does const_fold, but this is a decent way to check */
			die_at(&d->init->where, "not a constant expression");
	}

	if(d->type->primitive == type_void && !d->ptr_depth && !d->func)
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
			decl **diter;

			if(t->decls){
				decl **iter;

				for(iter = t->decls; *iter; iter++){
					symtab_add(t->symtab, *iter, sym_auto);
					fold_decl(*iter, t->symtab);
				}
			}

			auto_offset = t->symtab->parent ? t->symtab->parent->auto_offset : 0;
			arg_offset  = 0;

			for(diter = t->symtab->decls; diter && *diter; diter++){
				sym *s = (*diter)->sym;
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

void fold_func(decl *df, symtable *globsymtab)
{
	decl **diter;
	function *f = df->func;

	fold_decl(df, globsymtab);

	for(diter = df->func->args; diter && *diter; diter++)
		fold_decl(*diter, globsymtab);

	if(f->code){
		if(f->args){
			/* check for unnamed params */
			int nargs, i;

			for(nargs = 0; f->args[nargs]; nargs++);

			/* add args backwards, since we push them onto the stack backwards */
			for(i = nargs - 1; i >= 0; i--)
				if(!f->args[i]->spel)
					die_at(&f->where, "function \"%s\" has unnamed arguments", df->spel);
				else
					symtab_add(f->code->symtab, f->args[i], sym_arg);
		}

		symtab_nest(globsymtab, &f->code->symtab);

		fold_code(f->code);
	}else{
		decl **iter;
		int found = 0;

		for(iter = globsymtab->decls; iter && *iter; iter++)
			if((*iter)->func){
				function *f2 = (*iter)->func;

				if(f2->code && !strcmp((*iter)->spel, df->spel)){
					found = 1;
					break;
				}
			}

		if(!found){
			df->type->spec |= spec_extern;
			warn_at(&f->where, "assuming \"%s\" is extern", df->spel);
		}
	}
}

void fold(symtable *globs)
{
	decl **iter;

	/* extern removal */
	for(iter = globs->decls; iter && *iter; iter++){
		decl *d = *iter;

		if(d->type->spec & spec_extern){
			decl **iter2;

			for(iter2 = globs->decls; iter2 && *iter2; iter2++){
				decl *d2 = *iter2;
				if(!strcmp(d2->spel, d->spel) && (d2->type->spec & spec_extern) == 0){
					fprintf(stderr, "overriding extern \"%s\"\n", d->spel);
					d->ignore = 1;
					break;
				}
			}
		}
	}

	for(iter = globs->decls; iter && *iter; iter++){
		decl *d = *iter;

		if(d->ignore)
			continue;

		if(d->func){
			fold_func(*iter, globs);
		}else{
			fprintf(stderr, "fold_decl(\"%s\")\n", d->spel);
			fold_decl(*iter, globs);
		}
	}
}
