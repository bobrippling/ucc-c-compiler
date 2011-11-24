#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "cc1.h"
#include "../util/util.h"
#include "tree.h"
#include "fold.h"
#include "../util/util.h"
#include "sym.h"
#include "../util/platform.h"
#include "const.h"
#include "asm.h"

#define DIE_UNDECL_SPEL(sp) \
		die_at(&e->where, "undeclared identifier \"%s\" (%s:%d)", sp, __FILE__, __LINE__)

#define DIE_UNDECL() DIE_UNDECL_SPEL(e->spel)

void fold_expr(expr *e, symtable *stab);
void fold_code(tree *t);

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

#define GET_TREE_TYPE(from) \
	do{ \
		/*if(e->tree_type) \
			decl_free(e->tree_type);*/ \
		e->tree_type = decl_copy(from); \
		e->tree_type->spel = NULL; \
	}while(0)

void fold_funcall(expr *e, symtable *stab)
{
	decl *df;

	if(e->funcargs){
		expr **iter;
		for(iter = e->funcargs; *iter; iter++)
			fold_expr(*iter, stab);
	}

	if(!e->sym){
		df = decl_new_where(&e->where);

		df->func = function_new();

		df->type->primitive = type_int;
		df->spel = e->spel;

		warn_at(&e->where, "implicit declaration of function \"%s\"", e->spel);

		e->sym = symtab_add(symtab_grandparent(stab), df, sym_func);

		df->sym = NULL;
		/*
		 * fold() sets df->sym below,
		 * and doesn't expect it to be set.
		 * -> df is now part of the global sym table,
		 * having just been added via grandparent()
		 */

		/* set up the function args to correspond with the arg types */
		if(e->funcargs){
			expr **iter;
			for(iter = e->funcargs; *iter; iter++)
				dynarray_add((void ***)&df->func->args, (*iter)->tree_type);
		}
	}else{
		df = e->sym->decl;
	}

	GET_TREE_TYPE(e->sym->decl); /* XXX: check */

	/* func count comparison, only if the func has arg-decls, or the func is f(void) */
	if(df->func->args || df->func->args_void){
		expr **iter_arg;
		decl **iter_decl;
		int count_decl, count_arg;
		int i;

		count_decl = count_arg = 0;

		for(iter_arg  = e->funcargs;    iter_arg  && *iter_arg;  iter_arg++,  count_arg++);
		for(iter_decl = df->func->args; iter_decl && *iter_decl; iter_decl++, count_decl++);

		if(count_decl != count_arg && (df->func->variadic ? count_arg < count_decl : 1)){
			die_at(&e->where, "too %s arguments to function %s",
					count_arg > count_decl ? "many" : "few",
					df->spel);
		}

		if(e->funcargs){
			for(i = 0, iter_decl = df->func->args, iter_arg = e->funcargs;
					iter_decl[i];
					i++){
				/* TODO: -fstrict-types - changes the below 0 to a 1 */
				if(!decl_equal(iter_arg[i]->tree_type, iter_decl[i], 0)){
					char buf[DECL_STATIC_BUFSIZ];
					void (*fn)(where *, const char *, ...);

					fn = iter_arg[i]->tree_type->type->primitive == type_void ? die_at : warn_at;

					strcpy(buf, decl_to_str(iter_arg[i]->tree_type));
					fn(&e->where, "mismatching arguments for arg %d to %s: got %s, expected %s",
							i + 1, df->spel, buf, decl_to_str(iter_decl[i]));
				}
			}
		}
	}
}

void fold_assignment(expr *e, symtable *stab)
{
	if(!fold_is_lvalue(e->lhs))
		die_at(&e->lhs->where, "not an lvalue");

	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);


	if(!e->sym){
		/* need this here, since the generic sym-assignment does it from ->spel and not with assigning_to either */
		e->sym = symtab_search(stab, e->lhs->spel);

		if(!e->sym)
			DIE_UNDECL_SPEL(e->lhs->spel);
	}


	/* read the tree_type from what we're assigning to, not the expr */
	GET_TREE_TYPE(e->sym->decl);

	/* type check */
	if(!decl_equal(e->lhs->tree_type, e->rhs->tree_type, 0)){
		char buf[DECL_STATIC_BUFSIZ];
		void (*fn)(where *, const char *, ...);

		strcpy(buf, decl_to_str(e->lhs->tree_type));

		if(e->lhs->tree_type->type->primitive == type_void ||
		   e->rhs->tree_type->type->primitive == type_void)
			fn = die_at;
		else
			fn = warn_at;

		fn(&e->where, "assignment type mismatch: got %s, expected %s",
				decl_to_str(e->rhs->tree_type), buf);
	}
}


void fold_expr(expr *e, symtable *stab)
{
	if(e->spel && !e->sym)
		e->sym = symtab_search(stab, e->spel);

	const_fold(e);

	switch(e->type){
		case expr_val:
			e->tree_type->type->primitive = type_int;
			break;

		case expr_sizeof:
			if(e->expr){
				fold_expr(e->expr, stab);
				GET_TREE_TYPE(e->expr->tree_type);
			}
			/* else ->tree_type has already been initialised */
			break;

		case expr_comma:
			fold_expr(e->lhs, stab);
			fold_expr(e->rhs, stab);
			GET_TREE_TYPE(e->rhs->tree_type);
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
			fold_assignment(e, stab);
			break;

		case expr_op:
			fold_expr(e->lhs, stab);
			if(e->rhs)
				fold_expr(e->rhs, stab);

			if(e->rhs){
				enum {
					SIGNED, UNSIGNED
				} rhs, lhs;

				rhs = e->rhs->tree_type->type->spec & spec_unsigned ? UNSIGNED : SIGNED;
				lhs = e->lhs->tree_type->type->spec & spec_unsigned ? UNSIGNED : SIGNED;

				if(rhs != lhs){
#define SIGN_CONVERT(test_hs, assert_hs) \
					if(e->test_hs->type == expr_val && e->test_hs->val.i >= 0){ \
						/*                                              \
						 * assert(lhs == UNSIGNED);                     \
						 * vals default to signed, change to unsigned   \
						 */                                             \
						if(assert_hs != UNSIGNED)                       \
							ICE("signed-unsigned assumption failure");    \
                                                            \
						e->test_hs->tree_type->type->spec |= spec_unsigned; \
						goto noproblem;                                 \
					}

					SIGN_CONVERT(rhs, lhs)
					SIGN_CONVERT(lhs, rhs)

					warn_at(&e->where, "comparison between signed and unsigned");
				}
			}
noproblem:

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
				/* TODO: checks for pointer + pointer (invalid), etc etc */
				if(e->rhs && e->rhs->tree_type->ptr_depth)
					GET_TREE_TYPE(e->rhs->tree_type);
				else
					GET_TREE_TYPE(e->lhs->tree_type);
			}

			/* need to do this check _after_ we get the correct tree type */
			if((e->op == op_plus || e->op == op_minus) && e->tree_type->ptr_depth && e->rhs){
				/* we're dealing with pointers, adjust the amount we add by */

				if(e->lhs->tree_type->ptr_depth)
					/* lhs is the pointer, we're adding on rhs, hence multiply rhs by lhs's ptr size */
					e->rhs = expr_ptr_multiply(e->rhs, e->lhs->tree_type);
				else
					e->lhs = expr_ptr_multiply(e->lhs, e->rhs->tree_type);

				/* TODO: const_fold again */
			}
			break;

		case expr_array:
		{
			sym *sym;
			sym = symtab_add(stab, decl_new_where(&e->where), stab->parent ? sym_auto : sym_global);

			if(e->array_type == ARRAY_STR){
				e->tree_type->type->primitive = type_char;
			}else{
				int i;
				e->tree_type->type->primitive = type_int;

				for(i = 0; e->val.exprs[i]; i++){
					fold_expr(e->val.exprs[i], stab);
					if(!const_fold(e->val.exprs[i]))
						die_at(&e->val.exprs[i]->where, "array init not constant");
				}
			}

			e->tree_type->type->spec &= spec_static;
			e->tree_type->ptr_depth = 1;

			memcpy(sym->decl, e->tree_type, sizeof *sym->decl);

			sym->decl->spel = asm_array_label(e->array_type == ARRAY_STR);
			e->sym = sym;
			break;
		}

		case expr_funcall:
			fold_funcall(e, stab);
			break;
	}
#undef GET_TREE_TYPE
}

void fold_decl(decl *d, symtable *stab)
{
	int i;

	if(d->type->primitive == type_void && !d->ptr_depth && !d->func)
		die_at(&d->type->where, "can't have a void variable");

	/* type spec checks */
	if(d->type->spec & spec_extern && d->type->spec & spec_static)
		die_at(&d->type->where, "can't have static extern");

	if(d->type->spec & spec_signed && d->type->spec & spec_unsigned)
		die_at(&d->type->where, "can't have signed unsigned");

	for(i = 0; d->arraysizes && d->arraysizes[i]; i++){
		fold_expr(d->arraysizes[i], stab);

		if(!fold_expr_is_const(d->arraysizes[i]))
			die_at(&d->arraysizes[i]->where, "not a constant expression");
	}
}

void fold_decl_global(decl *d, symtable *stab)
{
	if(d->init){
		if(d->type->spec & spec_extern)
			/* only need this check for globals, since block-decls aren't initalised */
			die_at(&d->where, "externs can't be initalised");

		fold_expr(d->init, stab);

		if(const_fold(d->init))
			/* yes I know fold_expr does const_fold, but this is a decent way to check */
			die_at(&d->init->where, "not a constant expression (initialiser is %s)",
					expr_to_str(d->init->type));
	}

	fold_decl(d, stab);
}


void fold_block(tree *t)
{
	int auto_offset, arg_offset;

	if(t->decls){
		decl **iter;

		for(iter = t->decls; *iter; iter++){
			symtab_add(t->symtab, *iter, sym_auto);
			fold_decl(*iter, t->symtab);
		}
	}

	if(!t->symtab->parent)
		ICE("symtab has no parent");

	auto_offset = t->symtab->parent->auto_offset_start;
	arg_offset  = 0;

	/* need to walk backwards */
	if(t->symtab->decls){
		const int word_size = platform_word_size();
		decl **diter;

		for(diter = t->symtab->decls; *diter; diter++);

		for(diter--; diter >= t->symtab->decls; diter--){
			sym *s = (*diter)->sym;

			if(s->type == sym_auto){
				s->offset = auto_offset;

				/* TODO: optimise for chars / don't assume everything is an int */
				if(s->decl->arraysizes){
					/* should've been folded fully */
					decl dtmp;
					int i;

					memcpy(&dtmp, s->decl, sizeof dtmp);
					dtmp.ptr_depth--;

					for(i = 0; s->decl->arraysizes[i]; i++)
						auto_offset += s->decl->arraysizes[i]->val.i * decl_size(&dtmp);

					/* needs to be a multiple of word_size */
					if(auto_offset % word_size)
						auto_offset += word_size - auto_offset % word_size;

				}else{
					/* use sizeof(int) for single chars etc etc */
					auto_offset += word_size;
				}
			}else if(s->type == sym_arg){
				s->offset = arg_offset;
				arg_offset += word_size;
			}
		}
	}

	if(t->codes){
		tree **iter;
		int subtab_offsets = 0;

		for(iter = t->codes; *iter; iter++){
			int offset;

			symtab_nest(t->symtab, &(*iter)->symtab);

			/* assign so subtrees know the correct parent offset */
			(*iter)->symtab->auto_offset_start = auto_offset;
			fold_code(*iter);
			offset = (*iter)->symtab->auto_offset_finish - auto_offset;

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

		if(subtab_offsets > auto_offset)
			auto_offset = subtab_offsets;
	}

	t->symtab->auto_offset_finish = auto_offset;
}

void fold_code(tree *t)
{
	switch(t->type){
		case stat_break:
			ICE("break not coded yet");
			break;

		case stat_goto:
			ICE("goto not implemented");
			if(t->expr->type != expr_identifier)
				die_at(&t->expr->where, "not a label identifier");
			/*t->expr->sym = symtab_search(SYMTAB_LABEL, t->expr->spel); ???*/
			break;

		case stat_while:
		case stat_do:
		case stat_if:
			fold_expr(t->expr, t->symtab);

			symtab_nest(t->symtab, &t->lhs->symtab);
			fold_code(t->lhs);
			t->symtab->auto_offset_finish = t->lhs->symtab->auto_offset_finish;

			if(t->rhs){
				symtab_nest(t->symtab, &t->rhs->symtab);
				fold_code(t->rhs);

				if(t->rhs->symtab->auto_offset_finish > t->symtab->auto_offset_finish)
					t->symtab->auto_offset_finish = t->rhs->symtab->auto_offset_finish;
			}
			break;

		case stat_for:
#define FOLD_IF(x) if(x) fold_expr(x, t->symtab)
			FOLD_IF(t->flow->for_init);
			FOLD_IF(t->flow->for_while);
			FOLD_IF(t->flow->for_inc);
#undef FOLD_IF

			symtab_nest(t->symtab, &t->lhs->symtab);
			fold_code(t->lhs);
			t->symtab->auto_offset_finish = t->lhs->symtab->auto_offset_finish;
			break;

		case stat_code:
			fold_block(t);
			break;

		case stat_return:
			if(t->expr)
		case stat_expr:
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

		/* this is similar to the extern-ignore, but for overwriting function prototypes */
		if((df->type->spec & spec_extern) == 0){
			for(iter = globsymtab->decls; iter && *iter; iter++)
				if((*iter)->func){
					function *f2 = (*iter)->func;

					if(f2->code && !strcmp((*iter)->spel, df->spel)){
						found = 1;
						df->ignore = 1;
						break;
					}
				}

			if(!found)
				df->type->spec |= spec_extern;
				/*warn_at(&f->where, "assuming \"%s\" is extern", df->spel);*/
		}
	}
}

void fold(symtable *globs)
{
#define D(x) globs->decls[x]
	int i;

	for(i = 0; D(i); i++){
		int j;

		if(D(i)->sym)
			ICE("%s: sym already set for global variable \"%s\"", where_str(&D(i)->where), D(i)->spel);

		/* extern overwrite check */
		if(D(i)->type->spec & spec_extern)
			for(j = 0; D(j); j++)
				if(j != i && D(j)->spel && !strcmp(D(j)->spel, D(i)->spel) && (D(j)->type->spec & spec_extern) == 0)
					D(i)->ignore = 1;

		D(i)->sym = sym_new(D(i), sym_global);

		if(D(i)->func)
			fold_func(D(i), globs);
		else
			fold_decl_global(D(i), globs);
	}

#undef D
}
