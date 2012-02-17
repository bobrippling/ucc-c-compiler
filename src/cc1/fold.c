#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "cc1.h"
#include "fold.h"
#include "fold_sym.h"
#include "sym.h"
#include "../util/platform.h"
#include "const.h"
#include "asm.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "struct.h"

#define DIE_UNDECL_SPEL(sp) \
		die_at(&e->where, "undeclared identifier \"%s\" (%s:%d)", sp, __FILE__, __LINE__)

#define DIE_UNDECL() DIE_UNDECL_SPEL(e->spel)

#define EXPR_NON_VOID(e, s) \
	if(!e->tree_type->ptr_depth && e->tree_type->type->primitive == type_void) \
		die_at(&e->where, "%s requires non-void expression", s)

#define funcall_name(e) (e)->spel


static decl *curdecl_func;   /* for function-local labels */
static tree *curtree_switch; /* for case + default */
static tree *curtree_flow;   /* for break */

void fold_expr(expr *e, symtable *stab);
void fold_tree(tree *t);

int fold_is_lvalue(expr *e)
{
	/*
	 * valid lvaluess:
	 *
	 *   x = 5;
	 *   (cast)expr = 5;
	 *   *[above] = 5;
	 *
	 * also can't be const, checked in fold_assign (since we allow const inits)
	 */

	if(e->type == expr_identifier && !e->tree_type->func)
		return 1;

	if(e->type == expr_op)
		switch(e->op){
			case op_deref:
			case op_struct_ptr:
			case op_struct_dot:
				return 1;
			default:
				break;
		}

	if(e->type == expr_cast)
		return fold_is_lvalue(e->rhs);

	return 0;
}

int fold_is_callable(expr *e)
{
	return e->type == expr_identifier; /* TODO: extend to function pointer */
}

int fold_expr_is_addressable(expr *e)
{
	return e->type == expr_identifier;
}

#define GET_TREE_TYPE_TO(to, from) \
	do{ \
		/*if(e->tree_type) \
			decl_free(e->tree_type);*/ \
		to->tree_type = decl_copy(from); \
		to->tree_type->spel = NULL; \
	}while(0)

/* keep ->arraysizes, since we use it in decl_size() */

#define GET_TREE_TYPE(from) \
	GET_TREE_TYPE_TO(e, from)

void fold_decl_equal(decl *a, decl *b, where *w, enum warning warn,
		const char *errfmt, ...)
{
	if(!decl_equal(a, b, fopt_mode & FOPT_STRICT_TYPES)){
		/*char buf[DECL_STATIC_BUFSIZ];
		va_list l;

		strcpy(buf, decl_to_str(b));*/

		/* TODO: prefix with decl_to_str() */
		va_list l;

		va_start(l, errfmt);
		cc1_warn_atv(w, a->type->primitive == type_void && !a->ptr_depth, warn, errfmt, l);
		va_end(l);
	}
}

void fold_funcall(expr *e, symtable *stab)
{
	char *spel;
	decl *df;

	if(!fold_is_callable(e->expr))
		die_at(&e->expr->where, "expression %s not callable", expr_to_str(e->expr->type));

	if(e->funcargs){
		expr **iter;
		for(iter = e->funcargs; *iter; iter++)
			fold_expr(*iter, stab);
	}

	spel = funcall_name(e->expr);
	e->sym = symtab_search(stab, spel);
	if(!e->sym){
		df = decl_new_where(&e->where);

		df->func = function_new();

		df->type->primitive = type_int;
		df->spel = spel;

		cc1_warn_at(&e->where, 0, WARN_IMPLICIT_FUNC, "implicit declaration of function \"%s\"", funcall_name(e->expr));

		e->sym = SYMTAB_ADD(symtab_grandparent(stab), df, sym_func);

		df->sym = NULL;
		/*
		 * fold() sets df->sym below,
		 * and doesn't expect it to be set.
		 * -> df is now part of the global sym table,
		 * having just been added via grandparent()
		 */

		if(e->funcargs){
#ifdef IMPLICIT_FUNC_ARG_EXACT
			/* set up the function args to correspond with the arg types */
			expr **iter;
			for(iter = e->funcargs; *iter; iter++){
				decl *arg_decl = decl_copy((*iter)->tree_type);
				arg_decl->type->spec &= ~(spec_extern | spec_static); /* don't inherit these, obv. */
				dynarray_add((void ***)&df->func->args, arg_decl);
			}
#else
			/* set up the function args as if it's "x()" - i.e. any args */
			function_empty_args(df->func);
#endif
		}
	}else{
		df = e->sym->decl;
	}

	GET_TREE_TYPE(e->sym->decl);

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
			die_at(&e->where, "too %s arguments to function %s (got %d, need %d)",
					count_arg > count_decl ? "many" : "few",
					df->spel, count_arg, count_decl);
		}

		if(e->funcargs){
			for(i = 0, iter_decl = df->func->args, iter_arg = e->funcargs;
					iter_decl[i];
					i++){
				fold_decl_equal(iter_decl[i], iter_arg[i]->tree_type, &e->where,
						WARN_ARG_MISMATCH, "mismatching argument type for arg %d to %s",
						i + 1, df->spel);
			}
		}
	}
}

void fold_assignment(expr *e, symtable *stab)
{
	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);

	if(e->lhs->tree_type->arraysizes)
		die_at(&e->lhs->where, "can't assign to array yet");

	/* wait until we get the tree types, etc */
	if(!fold_is_lvalue(e->lhs))
		die_at(&e->lhs->where, "not an lvalue (%s)", expr_to_str(e->lhs->type));

	if(e->lhs->tree_type->type->spec & spec_const){
		/* allow const init: */
		if(e->lhs->sym->decl->init != e->rhs)
			die_at(&e->where, "can't modify const expression");
		/* otherwise it's the init expr, allow */
	}


	if(!e->sym && e->lhs->spel){
		/* need this here, since the generic sym-assignment does it from ->spel and not with assigning_to either */
		e->sym = symtab_search(stab, e->lhs->spel);

		if(!e->sym)
			DIE_UNDECL_SPEL(e->lhs->spel);

		/* read the tree_type from what we're assigning to, not the expr */
		GET_TREE_TYPE(e->sym->decl);
	}else{
		GET_TREE_TYPE(e->lhs->tree_type);
	}


	/* type check */
	fold_decl_equal(e->lhs->tree_type, e->rhs->tree_type,
		&e->where, WARN_ASSIGN_MISMATCH,
				"assignment type mismatch%s%s%s",
				e->lhs->spel ? " (" : "",
				e->lhs->spel ? e->lhs->spel : "",
				e->lhs->spel ? ")" : "");
}

#include "fold_op.c"

void fold_expr(expr *e, symtable *stab)
{
	if(e->spel && !e->sym)
		e->sym = symtab_search(stab, e->spel);

	const_fold(e);

	switch(e->type){
		case expr_sizeof:
			if(!e->expr->expr_is_sizeof)
				fold_expr(e->expr, stab);
			/* fall through - tree type int */
		case expr_val:
			e->tree_type->type->primitive = type_int;
			break;

		case expr_comma:
			fold_expr(e->lhs, stab);
			fold_expr(e->rhs, stab);
			GET_TREE_TYPE(e->rhs->tree_type);
			break;

		case expr_if:
			fold_expr(e->expr, stab);
			if(const_expr_is_const(e->expr))
				POSSIBLE_OPT(e->expr, "constant ?: expression");
			if(e->lhs)
				fold_expr(e->lhs, stab);
			fold_expr(e->rhs, stab);
			GET_TREE_TYPE(e->rhs->tree_type); /* TODO: check they're the same */
			break;

		case expr_cast:
			fold_expr(e->rhs, stab);
			if(e->rhs->type == expr_cast){
				/* FIXME: check (for *(int *)0 = 5;) */
				/* get rid of e->rhs, replace with e->rhs->rhs */
				expr *del = e->rhs;

				e->rhs = e->rhs->rhs;

				expr_free(del->lhs); /* the overridden cast */
				expr_free(del);
			}
			GET_TREE_TYPE(e->lhs->tree_type);
			break;

		case expr_addr:
			if(e->array_store){
				sym *array_sym;

				UCC_ASSERT(!e->sym, "symbol found when looking for array store");
				UCC_ASSERT(!e->expr, "expression found in array store address-of");

				e->tree_type->type->spec |= spec_static;
				e->tree_type->ptr_depth = 1;

				array_sym = SYMTAB_ADD(symtab_grandparent(stab), decl_new_where(&e->where), stab->parent ? sym_local : sym_global);
				memcpy(array_sym->decl, e->tree_type, sizeof *array_sym->decl);

				e->spel =
				e->array_store->label =
				array_sym->decl->spel =
					asm_label_array(e->array_store->type == array_str);

				array_sym->decl->arrayinit = e->array_store;


				switch(e->array_store->type){
					case array_str:
						e->tree_type->type->primitive = type_char;
						break;

					case array_exprs:
					{
						expr **inits;
						int i;

						e->tree_type->type->primitive = type_int;

						inits = e->array_store->data.exprs;

						for(i = 0; inits[i]; i++){
							fold_expr(inits[i], stab);
							if(const_fold(inits[i]))
								die_at(&inits[i]->where, "array init not constant (%s)", expr_to_str(inits[i]->type));
						}
					}
				}

			}else{
				fold_expr(e->expr, stab);
				if(!fold_expr_is_addressable(e->expr))
					die_at(&e->expr->where, "can't take the address of %s", expr_to_str(e->expr->type));
				e->tree_type->ptr_depth++;
			}
			break;

		case expr_identifier:
			if(!e->sym){
				if(!strcmp(e->spel, "__func__")){
					/* mutate into a string literal */
					e->type = expr_addr;
					e->array_store = array_decl_new();

					e->array_store->data.str = curdecl_func->spel;
					e->array_store->len = strlen(curdecl_func->spel) + 1; /* +1 - take the null byte */

					e->array_store->type = array_str;

					fold_expr(e, stab);
					break;

				}else{
					DIE_UNDECL();
				}
			}

			GET_TREE_TYPE(e->sym->decl);
			break;

		case expr_assign:
			fold_assignment(e, stab);
			break;

		case expr_op:
			fold_op(e, stab);
			break;

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

#define SPEC(x) (d->type->spec & (x))

	/* type spec checks */
	if(SPEC(spec_extern) && SPEC(spec_static))
		die_at(&d->type->where, "can't have static extern");

	if(SPEC(spec_signed) && SPEC(spec_unsigned))
		die_at(&d->type->where, "can't have signed unsigned");

	if(SPEC(spec_auto)){
#define CANT_HAVE(sp) \
		if(SPEC(sp)) \
			die_at(&d->type->where, "can't have auto %s", spec_to_str(sp))

		CANT_HAVE(spec_static);
		CANT_HAVE(spec_extern);

#undef CANT_HAVE
	}

#undef SPEC

	for(i = 0; d->arraysizes && d->arraysizes[i]; i++){
		fold_expr(d->arraysizes[i], stab);

		if(!const_expr_is_const(d->arraysizes[i]))
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

void fold_tree_nest(tree *paren, tree *child)
{
	symtab_nest(paren->symtab, &child->symtab);
	fold_tree(child);
}

void fold_tree(tree *t)
{
	UCC_ASSERT(t->symtab->parent, "symtab has no parent");

	switch(t->type){
		case stat_break:
			if(!curtree_flow)
				die_at(&t->expr->where, "break outside a flow-control statement");

			t->expr = expr_new();
			t->expr->type = expr_identifier;
			t->expr->spel = curtree_flow->lblfin;
			break;

		case stat_goto:
		case stat_label:
		{
			char *save = t->expr->spel;
			if(t->expr->type != expr_identifier)
				die_at(&t->expr->where, "not a label identifier");
			/* else let the assembler check for link errors */
			t->expr->spel = asm_label_goto(t->expr->spel);
			free(save);

			if(t->type == stat_label)
				fold_tree_nest(t, t->lhs); /* compound */
			break;
		}

		case stat_while:
		case stat_do:
		{
			tree *oldflowtree = curtree_flow;
			curtree_flow = t;

			t->lblfin = asm_label_flowfin();

		case stat_if:
			fold_expr(t->expr, t->symtab);
			EXPR_NON_VOID(t->expr, stat_to_str(t->type));

			OPT_CHECK(t->expr, "constant expression in if/while");

			fold_tree_nest(t, t->lhs);

			if(t->type != stat_if)
				curtree_flow = oldflowtree;
			else if(t->rhs)
				fold_tree_nest(t, t->rhs);
			break;
		}

		case stat_for:
		{
			tree *oldflowtree = curtree_flow;
			curtree_flow = t;

			t->lblfin = asm_label_flowfin();

#define FOLD_IF(x) if(x) fold_expr(x, t->symtab)
			FOLD_IF(t->flow->for_init);
			FOLD_IF(t->flow->for_while);
			FOLD_IF(t->flow->for_inc);
#undef FOLD_IF

			if(t->flow->for_while)
				EXPR_NON_VOID(t->flow->for_while, "for-while");

			OPT_CHECK(t->flow->for_while, "constant expression in for");

			fold_tree_nest(t, t->lhs);

			curtree_flow = oldflowtree;
			break;
		}


		case stat_case_range:
		{
			int l, r;

			fold_expr(t->expr,  t->symtab);
			fold_expr(t->expr2, t->symtab);

			if(const_fold(t->expr) || const_fold(t->expr2))
				die_at(&t->where, "case range not constant");
			if(!curtree_switch)
				die_at(&t->where, "not inside a switch statement");

			EXPR_NON_VOID(t->expr,  "case");
			EXPR_NON_VOID(t->expr2, "case");

			l = t->expr->val.i.val;
			r = t->expr2->val.i.val;

			if(l >= r)
				die_at(&t->where, "case range equal or inverse");

			t->expr->spel = asm_label_case(CASE_RANGE, l);
			goto case_add;
		}

		case stat_case:
		{
			int def;

			fold_expr(t->expr, t->symtab);

			EXPR_NON_VOID(t->expr, "case");

			if(const_fold(t->expr))
				die_at(&t->expr->where, "case expression not constant");
			if(!curtree_switch)
				die_at(&t->expr->where, "not inside a switch statement");
			/* fall */
		case stat_default:
			if((def = !t->expr)){
				t->expr = expr_new();
				t->expr->expr_is_default = 1;
			}
			t->expr->spel = asm_label_case(def ? CASE_DEF : CASE_CASE, t->expr->val.i.val);
case_add:
			fold_tree_nest(t, t->lhs); /* compound */
			dynarray_add((void ***)&curtree_switch->codes, t);
			break;
		}

		case stat_switch:
		{
			tree *oldswtree = curtree_switch;
			tree *oldflowtree = curtree_flow;

			curtree_switch = t;
			curtree_flow   = t;

			t->lblfin = asm_label_flowfin();

			fold_expr(t->expr, t->symtab);

			EXPR_NON_VOID(t->expr, "switch");

			OPT_CHECK(t->expr, "constant expression in switch");

			fold_tree_nest(t, t->lhs);
			/* FIXME: check for duplicate case values and at most, 1 default */

			curtree_switch = oldswtree;
			curtree_flow   = oldflowtree;
			break;
		}

		case stat_code:
			if(t->decls){
				decl **iter;

				for(iter = t->decls; *iter; iter++){
					decl *d = *iter;

					if(d->func && d->func->code)
						die_at(&d->func->code->where, "can't nest functions");

					fold_decl(d, t->symtab);

					SYMTAB_ADD(t->symtab, d, d->func ? sym_func : sym_local);
				}
			}

			if(t->codes){
				tree **iter;
				for(iter = t->codes; *iter; iter++)
					fold_tree_nest(t, *iter);
			}

			/* static folding */
			if(t->decls){
				decl **iter;

				for(iter = t->decls; *iter; iter++){
					decl *d = *iter;
					/*
					 * check static decls - after we fold,
					 * so we've linked the syms and can change ->spel
					 */
					if(d->type->spec & spec_static){
						char *save = d->spel;
						d->spel = asm_label_static_local(curdecl_func, d->spel);
						free(save);
					}
				}
			}
			break;

		case stat_return:
			if(t->expr){
				fold_expr(t->expr, t->symtab);
				EXPR_NON_VOID(t->expr, "return");
			}
			break;

		case stat_expr:
			fold_expr(t->expr, t->symtab);
			break;

		case stat_noop:
			break;
	}
}

void fold_func(decl *df, symtable *globsymtab)
{
	int i;
	decl **diter;
	function *f = df->func;

	curdecl_func = df;

	fold_decl(df, globsymtab);

	for(diter = df->func->args; diter && *diter; diter++)
		fold_decl(*diter, globsymtab);

	if(f->args)
		for(i = 0; f->args[i]; i++)
			if(f->args[i]->type->spec & (spec_static | spec_extern)){
				const char *sp = f->args[i]->spel;
				die_at(&f->where, "argument %d %s%s%sin function \"%s\" is static or extern",
						i + 1,
						sp ? "(" : "",
						sp ? sp  : "",
						sp ? ") " : "",
						df->spel);
			}

	if(f->code){
		if(f->args){
			/* check for unnamed params and extern/static specs */
			int nargs;

			for(nargs = 0; f->args[nargs]; nargs++);

			/* add args backwards, since we push them onto the stack backwards */
			for(i = nargs - 1; i >= 0; i--)
				if(!f->args[i]->spel)
					die_at(&f->where, "function \"%s\" has unnamed arguments", df->spel);
				else
					SYMTAB_ADD(f->code->symtab, f->args[i], sym_arg);
		}

		symtab_nest(globsymtab, &f->code->symtab);
		fold_tree(f->code);

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
				/*cc1_warn_at(&f->where, 0, WARN_EXTERN_ASSUME, "assuming \"%s\" is extern", df->spel);*/
		}
	}

	curdecl_func = NULL;
}

int fold_struct(struc *st)
{
	int offset;
	decl **i;

	for(offset = 0, i = st->members; *i; i++){
		decl *d = *i;
		if(d->type->struc){
			d->struct_offset = offset;
			offset += fold_struct(d->type->struc);
		}else{
			d->struct_offset = offset;
			offset += decl_size(d);
		}
	}

	return offset;
}

void fold(symtable *globs)
{
#define D(x) globs->decls[x]
	int i;

	if(fopt_mode & FOPT_ENABLE_ASM){
		decl *d;
		function *f;
		d = decl_new();
		f = d->func = function_new();
		d->spel = ustrdup(ASM_INLINE_FNAME);


		f->args = umalloc(2 * sizeof *f->args);
		f->args[0] = decl_new();
		f->args[1] = NULL;
		f->args[0]->type->primitive = type_char;
		f->args[0]->type->spec     |= spec_const;
		f->args[0]->ptr_depth = 1;

		symtab_add(globs, d, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);
	}

	{
		/* FIXME: when struct decls are local to blocks, this will need moving */
		struc **it;
		for(it = globs->structs; it && *it; it++)
			fold_struct(*it);
	}

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
