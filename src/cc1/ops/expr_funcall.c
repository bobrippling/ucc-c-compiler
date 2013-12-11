#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_funcall.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../../util/alloc.h"
#include "../funcargs.h"
#include "../format_chk.h"

const char *str_expr_funcall()
{
	return "funcall";
}

static void sentinel_check(where *w, type_ref *ref, expr **args,
		const int variadic, const int nstdargs, symtable *stab)
{
#define ATTR_WARN_RET(w, ...) do{ warn_at(w, __VA_ARGS__); return; }while(0)

	decl_attr *attr = type_attr_present(ref, attr_sentinel);
	int i, nvs;
	expr *sentinel;

	if(!attr)
		return;

	if(!variadic)
		return; /* warning emitted elsewhere, on the decl */

	if(attr->attr_extra.sentinel){
		consty k;

		FOLD_EXPR(attr->attr_extra.sentinel, stab);
		const_fold(attr->attr_extra.sentinel, &k);

		if(k.type != CONST_VAL)
			die_at(&attr->where, "sentinel attribute not reducible to integer constant");

		i = k.bits.iv.val;
	}else{
		i = 0;
	}

	nvs = dynarray_count(args) - nstdargs;

	if(nvs == 0)
		ATTR_WARN_RET(w, "not enough variadic arguments for a sentinel");

	UCC_ASSERT(nvs >= 0, "too few args");

	if(i >= nvs)
		ATTR_WARN_RET(w, "sentinel index is not a variadic argument");

	sentinel = args[(nstdargs + nvs - 1) - i];

	if(!expr_is_null_ptr(sentinel, 0))
		ATTR_WARN_RET(&sentinel->where, "sentinel argument expected (got %s)",
				type_ref_to_str(sentinel->tree_type));

#undef ATTR_WARN_RET
}

static void static_array_check(
		decl *arg_decl, expr *arg_expr)
{
	/* if ty_func is x[static %d], check counts */
	type_ref *ty_expr = arg_expr->tree_type;
	type_ref *ty_decl = decl_is_decayed_array(arg_decl);
	consty k_decl;

	if(!ty_decl || !ty_decl->bits.ptr.is_static || !ty_decl->bits.ptr.size)
		return;

	if(expr_is_null_ptr(arg_expr, 1 /* int */)){
		warn_at(&arg_expr->where, "passing null-pointer where array expected");
		return;
	}

	const_fold(ty_decl->bits.ptr.size, &k_decl);

	if((ty_expr = type_ref_is_decayed_array(ty_expr))){
		/* ty_expr is the type_ref_ptr, decayed from array */
		if(ty_expr->bits.ptr.size){
			consty k_arg;

			const_fold(ty_expr->bits.ptr.size, &k_arg);

			if(k_decl.type == CONST_VAL && k_arg.bits.iv.val < k_decl.bits.iv.val)
				warn_at(&arg_expr->where,
						"array of size %" INTVAL_FMT_D
						" passed where size %" INTVAL_FMT_D " needed",
						k_arg.bits.iv.val, k_decl.bits.iv.val);
		}
	}
	/* else it's a random pointer, just be quiet */
}

void fold_expr_funcall(expr *e, symtable *stab)
{
	type_ref *type_func;
	funcargs *args_from_decl;
	char *sp = NULL;
	int count_decl = 0;
	char *desc;

#if 0
	if(func_is_asm(sp)){
		expr *arg1;
		const char *str;
		int i;

		if(!e->funcargs || e->funcargs[1] || !expr_kind(e->funcargs[0], addr))
			die_at(&e->where, "invalid __asm__ arguments");

		arg1 = e->funcargs[0];
		str = arg1->data_store->data.str;
		for(i = 0; i < arg1->array_store->len - 1; i++){
			char ch = str[i];
			if(!isprint(ch) && !isspace(ch))
invalid:
				die_at(&arg1->where, "invalid __asm__ string (character 0x%x at index %d, %d / %d)",
						ch, i, i + 1, arg1->array_store->len);
		}

		if(str[i])
			goto invalid;

		/* TODO: allow a long return, e.g. __asm__(("movq $5, %rax")) */
		e->tree_type = decl_new_void();
		return;
		ICE("TODO: __asm__");
	}
#endif


	if(!e->expr->in_parens && expr_kind(e->expr, identifier) && (sp = e->expr->bits.ident.spel)){
		/* check for implicit function */
		if(!(e->expr->bits.ident.sym = symtab_search(stab, sp))){
			funcargs *args = funcargs_new();
			decl *df;

			/* set up the funcargs as if it's "x()" - i.e. any args */
			funcargs_empty(args);

			type_func = type_ref_new_func(
					type_ref_new_type(type_new_primitive(type_int)), args);

			cc1_warn_at(&e->expr->where, 0, WARN_IMPLICIT_FUNC,
					"implicit declaration of function \"%s\"", sp);

			df = decl_new();
			df->ref = type_func;
			df->spel = e->expr->bits.ident.spel;

			/* not declared - generate a sym ourselves */
			e->expr->bits.ident.sym = sym_new_stab(stab, df, sym_global);
		}
	}

	desc = ustrprintf("function argument to %s", sp);
	FOLD_EXPR(e->expr, stab);
	type_func = e->expr->tree_type;

	if(!type_ref_is_callable(type_func)){
		die_at(&e->expr->where, "%s-expression (type '%s') not callable",
				e->expr->f_str(), type_ref_to_str(type_func));
	}

	if(expr_kind(e->expr, deref)
	&& type_ref_is(type_ref_is_ptr(expr_deref_what(e->expr)->tree_type), type_ref_func)){
		/* XXX: memleak */
		/* (*f)() - dereffing to a function, then calling - remove the deref */
		e->expr = expr_deref_what(e->expr);
	}

	e->tree_type = type_ref_func_call(type_func, &args_from_decl);

	/* func count comparison, only if the func has arg-decls, or the func is f(void) */
	UCC_ASSERT(args_from_decl, "no funcargs for decl %s", sp);


	/* this block is purely count checking */
	if(args_from_decl->arglist || args_from_decl->args_void){
		const int count_arg  = dynarray_count(e->funcargs);

		count_decl = dynarray_count(args_from_decl->arglist);

		if(count_decl != count_arg && (args_from_decl->variadic ? count_arg < count_decl : 1)){
			die_at(&e->where, "too %s arguments to function %s (got %d, need %d)",
					count_arg > count_decl ? "many" : "few",
					sp, count_arg, count_decl);
		}
	}else if(args_from_decl->args_void_implicit && e->funcargs){
		warn_at(&e->where, "too many arguments to implicitly (void)-function");
	}

	/* this block folds the args and type-checks */
	if(e->funcargs){
		unsigned long nonnulls = 0;
		int i, j;
		decl_attr *da;

		if((da = type_attr_present(type_func, attr_nonnull)))
			nonnulls = da->attr_extra.nonnull_args;

		for(i = j = 0; e->funcargs[i]; i++){
			expr *arg = FOLD_EXPR(e->funcargs[i], stab);

			fold_need_expr(arg, desc, 0);
			fold_disallow_st_un(arg, desc);

			if((nonnulls & (1 << i)) && expr_is_null_ptr(arg, 1))
				warn_at(&arg->where, "null passed where non-null required (arg %d)", i + 1);
		}
	}

	/* this block is purely type checking */
	if(args_from_decl->arglist || args_from_decl->args_void){
		int count_arg;

		count_arg  = dynarray_count(e->funcargs);
		count_decl = dynarray_count(args_from_decl->arglist);

		if(count_decl != count_arg && (args_from_decl->variadic ? count_arg < count_decl : 1)){
			die_at(&e->where, "too %s arguments to function %s (got %d, need %d)",
					count_arg > count_decl ? "many" : "few",
					sp, count_arg, count_decl);
		}

		if(e->funcargs){
			int i;

			for(i = 0; ; i++){
				expr *arg      = e->funcargs[i];
				decl *decl_arg = args_from_decl->arglist[i];
				int eq;
				char arg_buf[TYPE_REF_STATIC_BUFSIZ];
				char exp_buf[TYPE_REF_STATIC_BUFSIZ];

				if(!decl_arg)
					break;

				eq = fold_type_ref_equal(
						decl_arg->ref, arg->tree_type, &arg->where,
						WARN_ARG_MISMATCH, 0,
						"mismatching argument %d to %s (%s <-- %s)",
						i, sp,
						type_ref_to_str_r(exp_buf, decl_arg->ref),
						type_ref_to_str_r(arg_buf, arg->tree_type));

				if(!eq){
					fold_insert_casts(decl_arg->ref, &e->funcargs[i],
							stab, &arg->where, desc);

					arg = e->funcargs[i];
				}

				/* f(int [static 5]) check */
				static_array_check(decl_arg, arg);
			}
		}
	}

	free(desc), desc = NULL;

	/* each arg needs casting up to int size, if smaller */
	if(e->funcargs){
		int i;
		for(i = 0; e->funcargs[i]; i++)
			expr_promote_int_if_smaller(&e->funcargs[i], stab);
	}

	fold_disallow_st_un(e, "return");

	/* attr */
	{
		type_ref *r = e->expr->tree_type;

		format_check_call(
				&e->where, r,
				e->funcargs, args_from_decl->variadic);

		sentinel_check(
				&e->where, r,
				e->funcargs, args_from_decl->variadic,
				count_decl, stab);
	}

	/* check the subexp tree type to get the funcall decl_attrs */
	if(expr_attr_present(e->expr, attr_warn_unused))
		e->freestanding = 0; /* needs use */
}

void gen_expr_funcall(expr *e)
{
	if(0){
		out_comment("start manual __asm__");
		ICE("same");
#if 0
		fprintf(cc_out[SECTION_TEXT], "%s\n", e->funcargs[0]->data_store->data.str);
#endif
		out_comment("end manual __asm__");
	}else{
		/* continue with normal funcall */
		int nargs = 0;

		gen_expr(e->expr);

		if(e->funcargs){
			expr **aiter;

			for(aiter = e->funcargs; *aiter; aiter++, nargs++);

			for(aiter--; aiter >= e->funcargs; aiter--){
				expr *earg = *aiter;

				/* should be of size int or larger (for integral types)
				 * or double (for floating types)
				 */
				gen_expr(earg);
			}
		}

		out_call(nargs, e->tree_type, e->expr->tree_type);
	}
}

void gen_expr_str_funcall(expr *e)
{
	expr **iter;

	idt_printf("funcall, calling:\n");

	gen_str_indent++;
	print_expr(e->expr);
	gen_str_indent--;

	if(e->funcargs){
		int i;
		idt_printf("args:\n");
		gen_str_indent++;
		for(i = 1, iter = e->funcargs; *iter; iter++, i++){
			idt_printf("arg %d:\n", i);
			gen_str_indent++;
			print_expr(*iter);
			gen_str_indent--;
		}
		gen_str_indent--;
	}else{
		idt_printf("no args\n");
	}
}

void mutate_expr_funcall(expr *e)
{
	(void)e;
}

int expr_func_passable(expr *e)
{
	/* need to check the sub-expr, i.e. the function */
	return !expr_attr_present(e->expr, attr_noreturn);
}

expr *expr_new_funcall()
{
	expr *e = expr_new_wrapper(funcall);
	e->freestanding = 1;
	return e;
}

void gen_expr_style_funcall(expr *e)
{
	stylef("(");
	gen_expr(e->expr);
	stylef(")(");
	if(e->funcargs){
		expr **i;
		for(i = e->funcargs; i && *i; i++){
			gen_expr(*i);
			if(i[1])
				stylef(", ");
		}
	}
	stylef(")");
}
