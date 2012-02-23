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
#include "enum.h"
#include "struct_enum.h"

#define DIE_UNDECL_SPEL(sp) \
		die_at(&e->where, "undeclared identifier \"%s\" (%s:%d)", sp, __FILE__, __LINE__)

#define DIE_UNDECL() DIE_UNDECL_SPEL((e)->spel)

#define EXPR_NON_VOID(e, s) \
	if(!decl_ptr_depth(e->tree_type) && e->tree_type->type->primitive == type_void) \
		die_at(&e->where, "%s requires non-void expression", s)

static char *curdecl_func_sp;    /* for funcargs-local labels */
static tree *curtree_switch;     /* for case + default */
static tree *curtree_flow;       /* for break */

void fold_expr(expr *e, symtable *stab);
void fold_funcargs(funcargs *fargs, symtable *stab, char *context);
void fold_tree(tree *t);

int fold_is_lvalue(expr *e)
{
	/*
	 * valid lvaluess:
	 *
	 *   x              = 5;
	 *   *(expr)        = 5;
	 *   struct.member  = 5;
	 *   struct->member = 5;
	 * and so on
	 *
	 * also can't be const, checked in fold_assign (since we allow const inits)
	 */

	if(e->type == expr_identifier)
		return !e->tree_type->func_code;

	if(e->type == expr_op)
		switch(e->op){
			case op_deref:
			case op_struct_ptr:
			case op_struct_dot:
				return 1;
			default:
				break;
		}

	return 0;
}

int fold_expr_is_addressable(expr *e)
{
	return e->type == expr_identifier;
}

#define GET_TREE_TYPE_TO(to, from) \
	do{ \
		to->tree_type = decl_copy(from); \
	}while(0)

#define GET_TREE_TYPE(from) \
	GET_TREE_TYPE_TO(e, from)

void fold_decl_equal(decl *a, decl *b, where *w, enum warning warn,
		const char *errfmt, ...)
{
	if(!decl_equal(a, b, fopt_mode & FOPT_STRICT_TYPES)){
		char buf[DECL_STATIC_BUFSIZ];
		va_list l;

		strcpy(buf, decl_to_str(b));

		cc1_warn_at(w, 0, warn, "%s vs. %s for...", decl_to_str(a), buf);

		va_start(l, errfmt);
		cc1_warn_atv(w, a->type->primitive == type_void && !decl_ptr_depth(a), warn, errfmt, l);
		va_end(l);
	}
}

void fold_funcall(expr *e, symtable *stab)
{
	decl *df;
	funcargs *args_exp;

	if(e->expr->type == expr_identifier && e->expr->spel){
		char *const sp = e->expr->spel;

		e->sym = symtab_search(stab, sp);
		if(!e->sym){
			df = decl_new_where(&e->where);

			df->type->primitive = type_int;
			df->type->spec     |= spec_extern;

			cc1_warn_at(&e->where, 0, WARN_IMPLICIT_FUNC, "implicit declaration of function \"%s\"", sp);

			df->spel = sp;

			df->funcargs = funcargs_new();

			if(e->funcargs)
				/* set up the funcargs as if it's "x()" - i.e. any args */
				function_empty_args(df->funcargs);

			e->sym = symtab_add(symtab_root(stab), df, sym_global, SYMTAB_WITH_SYM, SYMTAB_PREPEND);
		}else{
			df = e->sym->decl;
		}

		fold_expr(e->expr, stab);
	}else{
		fold_expr(e->expr, stab);

		df = e->expr->tree_type;

		if(!decl_is_callable(df)){
			die_at(&e->expr->where, "expression %s (%s) not callable",
					expr_to_str(e->expr->type),
					decl_to_str(df));
		}
	}

	GET_TREE_TYPE(df);
	/*
	 * TODO:
	 *
	 * int (*x)();
	 * (*x)();
	 * evaluates to tree_type = int;
	 */

	if(e->funcargs){
		expr **iter;
		for(iter = e->funcargs; *iter; iter++)
			fold_expr(*iter, stab);
	}

	/* func count comparison, only if the func has arg-decls, or the func is f(void) */
	args_exp = df->funcargs;

	if(!args_exp){
		/*
		 * int f(); - has funcargs on the decl
		 * int (*x)(); - funcargs are on the first ->decl_ptr
		 */
		args_exp = df->decl_ptr->fptrargs;
	}

	UCC_ASSERT(args_exp, "no funcargs for decl %s", df->spel);

	if(args_exp->arglist || args_exp->args_void){
		expr **iter_arg;
		decl **iter_decl;
		int count_decl, count_arg;
		int i;

		count_decl = count_arg = 0;

		for(iter_arg  = e->funcargs;       iter_arg  && *iter_arg;  iter_arg++,  count_arg++);
		for(iter_decl = args_exp->arglist; iter_decl && *iter_decl; iter_decl++, count_decl++);

		if(count_decl != count_arg && (args_exp->variadic ? count_arg < count_decl : 1)){
			die_at(&e->where, "too %s arguments to function %s (got %d, need %d)",
					count_arg > count_decl ? "many" : "few",
					df->spel, count_arg, count_decl);
		}

		if(e->funcargs){
			for(i = 0, iter_decl = args_exp->arglist, iter_arg = e->funcargs;
					iter_decl[i];
					i++){
				char a[DECL_STATIC_BUFSIZ], b[DECL_STATIC_BUFSIZ];

				strcpy(a, decl_to_str(iter_decl[i]));
				strcpy(b, decl_to_str(iter_arg[i]->tree_type));

				fold_decl_equal(iter_decl[i], iter_arg[i]->tree_type, &e->where,
						WARN_ARG_MISMATCH, "mismatching argument for arg %d to %s (%s vs. %s)",
						i + 1, df->spel, a, b);
			}
		}
	}
}

void fold_assignment(expr *e, symtable *stab)
{
	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);

	/* wait until we get the tree types, etc */
	if(!fold_is_lvalue(e->lhs))
		die_at(&e->lhs->where, "not an lvalue (%s%s%s)",
				expr_to_str(e->lhs->type),
				e->lhs->type == expr_op ? " - " : "",
				e->lhs->type == expr_op ? op_to_str(e->lhs->op) : ""
			);

	if(decl_is_const(e->lhs->tree_type)){
		/* allow const init: */
		if(e->lhs->sym->decl->init != e->rhs)
			die_at(&e->where, "can't modify const expression");
	}


	if(e->lhs->sym)
		/* read the tree_type from what we're assigning to, not the expr */
		GET_TREE_TYPE(e->lhs->sym->decl);
	else
		GET_TREE_TYPE(e->lhs->tree_type);


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

				/* static const char * */
				*decl_leaf(e->tree_type) = decl_ptr_new();

				e->tree_type->type->spec |= spec_static | spec_const;
				e->tree_type->type->primitive = type_char;

				e->spel = e->array_store->label = asm_label_array(e->array_store->type == array_str);

				e->tree_type->spel = e->spel;

				array_sym = SYMTAB_ADD(symtab_root(stab), e->tree_type, stab->parent ? sym_local : sym_global);

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

				GET_TREE_TYPE(e->expr->sym ? e->expr->sym->decl : e->expr->tree_type);

				e->tree_type = decl_ptr_depth_inc(e->tree_type);
			}
			break;

		case expr_identifier:
			if(!e->sym){
				if(!strcmp(e->spel, "__func__")){
					/* mutate into a string literal */
					e->type = expr_addr;
					e->array_store = array_decl_new();

					UCC_ASSERT(curdecl_func_sp, "no spel for current func");
					e->array_store->data.str = curdecl_func_sp;
					e->array_store->len = strlen(curdecl_func_sp) + 1; /* +1 - take the null byte */

					e->array_store->type = array_str;

					fold_expr(e, stab);
					break;

				}else{
					/* check for an enum */
					enum_member *m = enum_find_member(stab, e->spel);

					if(!m)
						DIE_UNDECL_SPEL(e->spel);

					e->type = expr_val;
					e->val = m->val->val;
					fold_expr(e, stab);
					goto fin;
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

fin:
	UCC_ASSERT(e->tree_type->type->primitive != type_unknown, "unknown type after folding expr %s", expr_to_str(e->type));
}

void fold_decl_ptr(decl_ptr *dp, symtable *stab, decl *root)
{
	if(dp->fptrargs)
		fold_funcargs(dp->fptrargs, stab, root->spel);

	if(dp->array_size){
		long v;
		fold_expr(dp->array_size, stab);
		if((v = dp->array_size->val.i.val) < 0)
			die_at(&dp->where, "negative array length");
	}

	if(dp->child)
		fold_decl_ptr(dp->child, stab, root);
}

void fold_decl(decl *d, symtable *stab)
{
	switch(d->type->primitive){
		int incomplete;

		case type_void:
			if(!decl_ptr_depth(d) && !d->funcargs)
				die_at(&d->type->where, "can't have a void variable (%s)", decl_to_str(d));
			break;

		case type_enum:
			st_en_lookup((void **)&d->type->enu,   &incomplete, d, stab, (void *(*)(struct symtable *, const char *))enum_find,   0);
			goto incomp_check;

		case type_struct:
			st_en_lookup((void **)&d->type->struc, &incomplete, d, stab, (void *(*)(struct symtable *, const char *))struct_find, 1);
incomp_check:
			if(incomplete && !decl_ptr_depth(d))
				die_at(&d->where, "use of incomplete type \"%s\"", d->spel);
			break;

		default:
			break;
	}

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

	if(d->decl_ptr)
		fold_decl_ptr(d->decl_ptr, stab, d);

#undef SPEC
}

void fold_decl_global(decl *d, symtable *stab)
{
	if(d->init){
		if(d->type->spec & spec_extern)
			/* only need this check for globals, since block-decls aren't initalised */
			die_at(&d->where, "externs can't be initalised");

		fold_expr(d->init, stab);

		if(const_fold(d->init))
			die_at(&d->init->where, "not a constant expression (initialiser is %s)",
					expr_to_str(d->init->type));
	}else if(d->type->spec & spec_extern){
		/* we have an extern, check if it's overridden */
		char *const spel = d->spel;
		decl **dit;

		for(dit = stab->decls; dit && *dit; dit++){
			decl *d2 = *dit;
			if(!strcmp(d2->spel, spel) && (d2->type->spec & spec_extern) == 0){
				/* found an override */
				d->ignore = 1;
				break;
			}
		}
	}

	fold_decl(d, stab);
}

int fold_struct(struct_st *st)
{
	int offset;
	decl **i;

	for(offset = 0, i = st->members; *i; i++){
		decl *d = *i;
		if(d->type->primitive == type_struct && !d->type->struc)
			ICE("TODO: struct lookup");

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

void fold_enum(enum_st *en, symtable *stab)
{
	enum_member **i;
	int defval = 0;

	for(i = en->members; *i; i++){
		enum_member *m = *i;
		expr *e = m->val;

		if(e == (expr *)-1){
			/*expr_free(e); XXX: memleak */
			m->val = expr_new_val(defval++);
		}else{
			fold_expr(e, stab);
			if(!const_expr_is_const(e))
				die_at(&e->where, "enum value not constant");
			defval = const_expr_val(e) + 1;
		}
	}
}

void fold_symtab_scope(symtable *stab)
{
	struct_st **sit;
	enum_st   **eit;

	/* fold structs, then enums, then decls - decls may rely on enums */
	for(sit = stab->structs; sit && *sit; sit++)
		fold_struct(*sit);

	for(eit = stab->enums; eit && *eit; eit++)
		fold_enum(*eit, stab);
}

void fold_switch_enum(tree *sw, type *enum_type)
{
	const int nents = enum_nentries(enum_type->enu);
	tree **titer;
	char *marks = umalloc(nents * sizeof *marks);
	int midx;

	/* for each case/default/case_range... */
	for(titer = sw->codes; titer && *titer; titer++){
		tree *cse = *titer;
		int v, w;

		if(cse->expr->expr_is_default)
			goto ret;

		v = cse->expr->val.i.val;

		if(cse->type == stat_case_range)
			w = cse->expr2->val.i.val;
		else
			w = v;

		for(; v <= w; v++){
			enum_member **mi;
			for(midx = 0, mi = enum_type->enu->members; *mi; midx++, mi++)
				if(v == (*mi)->val->val.i.val)
					marks[midx]++;
		}
	}

	for(midx = 0; midx < nents; midx++)
		if(!marks[midx])
			cc1_warn_at(&sw->where, 0, WARN_SWITCH_ENUM, "enum %s not handled in switch", enum_type->enu->members[midx]->spel);

ret:
	free(marks);
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
			t->expr->tree_type->type->primitive = type_int;
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
				fold_tree(t->lhs); /* compound */
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

			fold_tree(t->lhs);

			if(t->type != stat_if)
				curtree_flow = oldflowtree;
			else if(t->rhs)
				fold_tree(t->rhs);
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

			fold_tree(t->lhs);

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
			/* fall */
		case stat_default:
			if((def = !t->expr)){
				t->expr = expr_new();
				t->expr->expr_is_default = 1;
			}
			t->expr->spel = asm_label_case(def ? CASE_DEF : CASE_CASE, t->expr->val.i.val);
case_add:
			fold_tree(t->lhs); /* compound */
			if(!curtree_switch)
				die_at(&t->expr->where, "not inside a switch statement");
			dynarray_add((void ***)&curtree_switch->codes, t);
			break;
		}

		case stat_switch:
		{
			tree *oldswtree = curtree_switch;
			tree *oldflowtree = curtree_flow;
			type *typ;

			curtree_switch = t;
			curtree_flow   = t;

			t->lblfin = asm_label_flowfin();

			fold_expr(t->expr, t->symtab);

			EXPR_NON_VOID(t->expr, "switch");

			OPT_CHECK(t->expr, "constant expression in switch");

			fold_tree(t->lhs);
			/* FIXME: check for duplicate case values and at most, 1 default */

			/* check for an enum */
			typ = t->expr->tree_type->type;
			if(typ->primitive == type_enum){
				UCC_ASSERT(typ->enu, "no enum for enum type");
				fold_switch_enum(t, typ);
			}

			curtree_switch = oldswtree;
			curtree_flow   = oldflowtree;
			break;
		}

		case stat_code:
		{
			decl **iter;

			fold_symtab_scope(t->symtab);

			for(iter = t->decls; iter && *iter; iter++){
				decl *d = *iter;

				if(d->func_code)
					die_at(&d->func_code->where, "can't nest functions");

				fold_decl(d, t->symtab);

				SYMTAB_ADD(t->symtab, d, sym_local);
			}

			if(t->codes){
				tree **iter;
				for(iter = t->codes; *iter; iter++)
					fold_tree(*iter);
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
						d->spel = asm_label_static_local(curdecl_func_sp, d->spel);
						free(save);
					}
				}
			}
			break;
		}

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

void fold_funcargs(funcargs *fargs, symtable *stab, char *context)
{
	decl **diter;

	for(diter = fargs->arglist; diter && *diter; diter++)
		fold_decl(*diter, stab);

	if(fargs->arglist){
		/* check for unnamed params and extern/static specs */
		int i;

		for(i = 0; fargs->arglist[i]; i++){
			if(fargs->arglist[i]->type->spec & (spec_static | spec_extern)){
				const char *sp = fargs->arglist[i]->spel;
				die_at(&fargs->where, "argument %d %s%s%sin function \"%s\" is static or extern",
						i + 1,
						sp ? "(" : "",
						sp ? sp  : "",
						sp ? ") " : "",
						context);
			}
		}
	}
}

void fold_func(decl *func_decl, symtable *globs)
{
	if(func_decl->func_code){
		funcargs *fargs;

		curdecl_func_sp = func_decl->spel;

		fargs = func_decl->funcargs;
		UCC_ASSERT(fargs, "function %s has no funcargs", func_decl->spel);

		if(fargs->arglist){
			int nargs, i;

			for(nargs = 0; fargs->arglist[nargs]; nargs++);

			/* add args backwards, since we push them onto the stack backwards */
			for(i = nargs - 1; i >= 0; i--)
				if(!fargs->arglist[i]->spel)
					die_at(&fargs->where, "function \"%s\" has unnamed arguments", func_decl->spel);
				else
					SYMTAB_ADD(func_decl->func_code->symtab, fargs->arglist[i], sym_arg);
		}

		symtab_set_parent(func_decl->func_code->symtab, globs);

		fold_tree(func_decl->func_code);

		curdecl_func_sp = NULL;
	}
}

void fold(symtable *globs)
{
#define D(x) globs->decls[x]
	int i;

	if(fopt_mode & FOPT_ENABLE_ASM){
		decl *df;
		funcargs *fargs;

		df = decl_new();
		fargs = df->funcargs = funcargs_new();
		df->spel = ustrdup(ASM_INLINE_FNAME);

		df->type->primitive = type_int;

		fargs->arglist    = umalloc(2 * sizeof *fargs->arglist);
		fargs->arglist[0] = decl_new();
		fargs->arglist[1] = NULL;
		fargs->arglist[0]->type->primitive = type_char;
		fargs->arglist[0]->type->spec     |= spec_const;
		fargs->arglist[0]->decl_ptr        = decl_ptr_new();

		symtab_add(globs, df, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);
	}

	fold_symtab_scope(globs);

	for(i = 0; D(i); i++)
		if(D(i)->sym)
			ICE("%s: sym (%p) already set for global \"%s\"", where_str(&D(i)->where), D(i)->sym, D(i)->spel);

	for(;;){
		int i;

		/* find the next sym (since we can prepend, start at 0 each time */
		for(i = 0; D(i); i++)
			if(!D(i)->sym)
				break;

		if(!D(i))
			break; /* finished */

		D(i)->sym = sym_new(D(i), sym_global);

		fold_decl_global(D(i), globs);
		if(D(i)->func_code)
			fold_func(D(i), globs);
	}

	/*
	 * change int x(); to extern int x();
	 * if there is no decl->func_code for "x"
	 *
	 * otherwise, ignore it (since we have a func code for it elsewhere)
	 * e.g. int x(); int x(){}
	 */
	for(i = 0; D(i); i++){
		char *const spel_i = D(i)->spel;

		if(D(i)->funcargs && !D(i)->func_code){
			int found = 0;
			int j;

			for(j = 0; D(j); j++){
				if(j != i && D(j)->func_code && !strcmp(spel_i, D(j)->spel)){
					D(i)->ignore = 1;
					found = 1;
					break;
				}
			}

			if(!found){
				D(i)->type->spec |= spec_extern;
				/*cc1_warn_at(&f->where, 0, WARN_EXTERN_ASSUME, "assuming \"%s\" is extern", func_decl->spel);*/
			}
		}
	}

#undef D
}
