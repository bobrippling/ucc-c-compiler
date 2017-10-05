#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../../util/alloc.h"
#include "../../util/macros.h"

#include "ops.h"
#include "expr_funcall.h"
#include "../funcargs.h"
#include "../format_chk.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../c_funcs.h"

#define ARG_BUF(buf, i, sp)       \
	snprintf(buf, sizeof buf,       \
			"argument %d to %s",        \
			i + 1, sp ? sp : "function")

const char *str_expr_funcall()
{
	return "function-call";
}

static attribute *func_attr_present(expr *e, enum attribute_type t)
{
	attribute *a;
	a = expr_attr_present(e, t);
	if(a)
		return a;
	return expr_attr_present(e->expr, t);
}

static void sentinel_check(where *w, expr *e, expr **args,
		const int variadic, const int nstdargs, symtable *stab)
{
#define ATTR_WARN_RET(w, ...) \
	do{ cc1_warn_at(w, attr_sentinel, __VA_ARGS__); return; }while(0)

	attribute *attr = func_attr_present(e, attr_sentinel);
	int i, nvs;
	expr *sentinel;

	if(!attr)
		return;

	if(!variadic)
		return; /* warning emitted elsewhere, on the decl */

	if(attr->bits.sentinel){
		consty k;

		FOLD_EXPR(attr->bits.sentinel, stab);
		const_fold(attr->bits.sentinel, &k);

		if(k.type != CONST_NUM || !K_INTEGRAL(k.bits.num))
			die_at(&attr->where, "sentinel attribute not reducible to integer constant");

		i = k.bits.num.val.i;
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

	/* must be of a pointer type, printf("%p\n", 0) is undefined */
	if(!expr_is_null_ptr(sentinel, NULL_STRICT_ANY_PTR))
		ATTR_WARN_RET(&sentinel->where, "sentinel argument expected (got %s)",
				type_to_str(sentinel->tree_type));

#undef ATTR_WARN_RET
}

static void static_array_check(
		decl *arg_decl, expr *arg_expr)
{
	/* if ty_func is x[static %d], check counts */
	type *ty_expr = arg_expr->tree_type;
	type *ty_decl = decl_is_decayed_array(arg_decl);
	consty k_decl;

	if(!ty_decl)
		return;

	assert(ty_decl->type == type_array);
	if(!ty_decl->bits.array.is_static)
		return;

	/* want to check any pointer type */
	if(expr_is_null_ptr(arg_expr, NULL_STRICT_ANY_PTR)){
		cc1_warn_at(&arg_expr->where,
				attr_nonnull,
				"passing null-pointer where array expected");
		return;
	}

	if(!ty_decl->bits.array.size)
		return;

	const_fold(ty_decl->bits.array.size, &k_decl);

	if((ty_expr = type_is_decayed_array(ty_expr))){

		assert(ty_expr->type == type_array);

		if(ty_expr->bits.array.size){
			consty k_arg;

			const_fold(ty_expr->bits.array.size, &k_arg);

			if(k_decl.type == CONST_NUM
			&& K_INTEGRAL(k_arg.bits.num)
			&& k_arg.bits.num.val.i < k_decl.bits.num.val.i)
			{
				cc1_warn_at(&arg_expr->where,
						static_array_bad,
						"array of size %" NUMERIC_FMT_D
						" passed where size %" NUMERIC_FMT_D " needed",
						k_arg.bits.num.val.i, k_decl.bits.num.val.i);
			}
		}
	}
	/* else it's a random pointer, just be quiet */
}

static void check_implicit_funcall(expr *e, symtable *stab, char **psp)
{
	funcargs *args;
	decl *df;
	type *func_ty;

	if(e->expr->in_parens
	|| !expr_kind(e->expr, identifier)
	|| !((*psp) = e->expr->bits.ident.bits.ident.spel))
	{
		return;
	}

	/* check for implicit function */
	if((e->expr->bits.ident.bits.ident.sym = symtab_search(stab, *psp)))
		return;

	args = funcargs_new();

	/* set up the funcargs as if it's "x()" - i.e. any args */
	funcargs_empty(args);

	func_ty = type_func_of(
			type_nav_btype(cc1_type_nav, type_int),
			args,
			symtab_new(stab, &e->where) /*new symtable for args*/);

	cc1_warn_at(&e->expr->where, implicit_func,
			"implicit declaration of function \"%s\"", *psp);

	df = decl_new();
	df->ref = func_ty;
	df->spel = e->expr->bits.ident.bits.ident.spel;

	fold_decl(df, stab); /* update calling conv, for e.g. */

	df->sym->type = sym_global;

	/* ensure code-gen of implicit symbol, e.g. forward decl, etc */
	symtab_add_to_scope_pre(symtab_root(stab), df);

	e->expr->bits.ident.bits.ident.sym = df->sym;
	e->expr->tree_type = func_ty;
}

static int check_arg_counts(
		funcargs *args_from_decl,
		unsigned count_decl,
		expr **exprargs,
		expr *fnexpr, char *sp)
{
	where *const loc = &fnexpr->where;

	/* this block is purely count checking */
	if(!FUNCARGS_EMPTY_NOVOID(args_from_decl)){
		const unsigned count_arg  = dynarray_count(exprargs);

		if(count_decl != count_arg
		&& (args_from_decl->variadic ? count_arg < count_decl : 1))
		{
			decl *call_decl;
			/* may be args_old_proto but also args_void if copied from
			 * another prototype elsewhere */
			int warn = args_from_decl->args_old_proto
				&& !args_from_decl->args_void;

#define common_warning                                         \
					"too %s arguments to function %s%s(got %d, need %d)",\
					count_arg > count_decl ? "many" : "few",             \
					sp ? sp : "",                                        \
					sp ? " " : "",                                       \
					count_arg, count_decl

			if(warn){
				cc1_warn_at(loc, funcall_argcount, common_warning);
			}else{
				warn_at_print_error(loc, common_warning);
			}

#undef common_warning

			if((call_decl = expr_to_declref(fnexpr->expr, NULL)))
				note_at(&call_decl->where, "'%s' declared here", call_decl->spel);

			if(!warn){
				fold_had_error = 1;
				return 1;
			}
		}
	}else if(args_from_decl->args_void_implicit && exprargs){
		cc1_warn_at(loc, funcall_argcount,
				"too many arguments to implicitly (void)-function");
	}
	return 0;
}

static void check_arg_voidness_and_nonnulls(
		expr *callexpr, symtable *stab,
		funcargs *args_from_decl, unsigned count_decl,
		expr **exprargs, char *sp)
{
	/* this block folds the args and type-checks */
	unsigned long nonnulls = 0;
	unsigned i;
	attribute *da;

	if((da = func_attr_present(callexpr, attr_nonnull)))
		nonnulls = da->bits.nonnull_args;

	for(i = 0; exprargs[i]; i++){
		expr *arg = FOLD_EXPR(exprargs[i], stab);
		char buf[64];

		ARG_BUF(buf, i, sp);

		fold_check_expr(arg, 0, buf);

		if(i < count_decl && (nonnulls & (1 << i))
		&& type_is_ptr(args_from_decl->arglist[i]->ref)
		&& expr_is_null_ptr(arg, NULL_STRICT_INT))
		{
			cc1_warn_at(&arg->where,
					attr_nonnull,
					"null passed where non-null required (arg %d)",
					i + 1);
		}
	}
}

static void check_arg_types(
		funcargs *args_from_decl,
		expr **exprargs, symtable *stab,
		char *sp, where *const exprloc)
{
	if(exprargs && args_from_decl->arglist){
		int i;
		char buf[64];

		for(i = 0; ; i++){
			decl *decl_arg = args_from_decl->arglist[i];

			if(!decl_arg)
				break;

			if(!type_is_complete(decl_arg->ref)){
				char wbuf[WHERE_BUF_SIZ];
				warn_at_print_error(&decl_arg->where,
						"incomplete parameter type '%s'\n"
						"%s: note: in call here",
						type_to_str(decl_arg->ref),
						where_str_r(wbuf, exprloc));
				fold_had_error = 1;
			}

			/* exprargs[i] may be NULL - old style function */
			if(!exprargs[i])
				continue;

			ARG_BUF(buf, i, sp);

			fold_type_chk_and_cast_ty(
					decl_arg->ref, &exprargs[i],
					stab, &exprargs[i]->where,
					buf);

			/* f(int [static 5]) check */
			static_array_check(decl_arg, exprargs[i]);
		}
	}
}

static void default_promote_args(
		expr **args, unsigned count_decl, symtable *stab)
{
	/* each unspecified arg needs default promotion, (if smaller) */
	unsigned i;

	/* must walk up from zero, since args[count_decl] may be OOB,
	 * in the case of an old style function:
	 * f(i, j){ ... }
	 * f(1);
	 */
	for(i = 0; args[i]; i++)
		if(i >= count_decl)
			expr_promote_default(&args[i], stab);
}

static void check_standard_funcs(const char *name, expr **args)
{
	const size_t nargs = dynarray_count(args);

	if(!strcmp(name, "free") && nargs == 1){
		c_func_check_free(args[0]);
	}else{
		static const struct {
			const char *name;
			unsigned nargs;
			int szarg;
			int ptrargs[2];
		} memfuncs[] = {
			{ "memcpy", 3, 2, { 0, 1 } },
			{ "memset", 3, 2, { 0, -1 } },
			{ "memmove", 3, 2, { 0, 1 } },
			{ "memcmp", 3, 2, { 0, 1 } },

			{ 0 }
		};

		for(int i = 0; memfuncs[i].name; i++){
			if(nargs == memfuncs[i].nargs && !strcmp(name, memfuncs[i].name)){
				expr *ptrargs[countof(memfuncs[0].ptrargs) + 1] = { 0 };
				unsigned arg;

				for(arg = 0; arg < countof(memfuncs[0].ptrargs); arg++){
					if(memfuncs[i].ptrargs[arg] == -1)
						break;

					ptrargs[arg] = args[memfuncs[i].ptrargs[arg]];
				}

				c_func_check_mem(ptrargs, args[memfuncs[i].szarg], memfuncs[i].name);
				break;
			}
		}
	}
}

void fold_expr_funcall(expr *e, symtable *stab)
{
	type *func_ty;
	funcargs *args_from_decl;
	char *sp = NULL;
	unsigned count_decl;

	check_implicit_funcall(e, stab, &sp);

	FOLD_EXPR(e->expr, stab);
	func_ty = e->expr->tree_type;

	if(!type_is_callable(func_ty)){
		warn_at_print_error(&e->expr->where, "%s-expression (type '%s') not callable",
				e->expr->f_str(), type_to_str(func_ty));

		fold_had_error = 1;

		e->tree_type = type_nav_btype(cc1_type_nav, type_int);
		return;
	}

	e->tree_type = type_func_call(func_ty, &args_from_decl);

	/* func count comparison, only if the func has arg-decls, or the func is f(void) */
	UCC_ASSERT(args_from_decl, "no funcargs for decl %s", sp);

	count_decl = dynarray_count(args_from_decl->arglist);

	if(check_arg_counts(args_from_decl, count_decl, e->funcargs, e, sp))
		return;

	if(e->funcargs){
		check_arg_voidness_and_nonnulls(
				e, stab,
				args_from_decl, count_decl,
				e->funcargs, sp);
	}

	if(!FUNCARGS_EMPTY_NOVOID(args_from_decl))
		check_arg_types(args_from_decl, e->funcargs, stab, sp, &e->where);

	if(e->funcargs)
		default_promote_args(e->funcargs, count_decl, stab);

	if(type_is_s_or_u(e->tree_type)){
		/* handled transparently by the backend */
		e->f_islval = expr_is_lval_struct;
	}

	/* attr */
	{
		type *fnty = e->expr->tree_type;

		format_check_call(fnty, e->funcargs, args_from_decl->variadic);

		sentinel_check(
				&e->where, e,
				e->funcargs, args_from_decl->variadic,
				count_decl, stab);
	}

	/* check the subexp tree type to get the funcall attributes */
	if(func_attr_present(e, attr_warn_unused))
		e->freestanding = 0; /* needs use */

	if(sp && !(fopt_mode & FOPT_FREESTANDING))
		check_standard_funcs(sp, e->funcargs);
}

const out_val *gen_expr_funcall(const expr *e, out_ctx *octx)
{
	const out_val *fn_ret;

	if(0){
		out_comment(octx, "start manual __asm__");
		ICE("same");
#if 0
		fprintf(cc_out[SECTION_TEXT], "%s\n", e->funcargs[0]->data_store->data.str);
#endif
		out_comment(octx, "end manual __asm__");
	}else{
		/* continue with normal funcall */
		const out_val *fn, **args = NULL;

		fn = gen_expr(e->expr, octx);

		if(e->funcargs){
			expr **aiter;

			for(aiter = e->funcargs; *aiter; aiter++){
				expr *earg = *aiter;
				const out_val *arg;

				/* should be of size int or larger (for integral types)
				 * or double (for floating types)
				 */
				arg = gen_expr(earg, octx);
				dynarray_add(&args, arg);
			}
		}

		/* consumes fn and args */
		fn_ret = gen_call(e->expr, NULL, fn, args, octx, &e->expr->where);

		dynarray_free(const out_val **, args, NULL);

		if(!expr_func_passable(GEN_CONST_CAST(expr *, e)))
			out_ctrl_end_undefined(octx);
	}

	return fn_ret;
}

static irval *funcall_ir_correct_type(const expr *e, irval *fnv, irctx *ctx)
{
	funcargs *args = type_funcargs(type_is_ptr(e->expr->tree_type));
	unsigned casted_val;

	if(!args->args_old_proto){
		return fnv;
	}

	/* need to cast T(old_arg1, old_arg2, ...) to T(...) in order to call it
	 *
	 * unless we're calling it with no arguments, in which case, we need no cast
	 * (either way, ir removes the cast / it doesn't appear in code-gen, but this
	 * keeps the output tidy)
	 */
	if(!args->arglist && !e->funcargs){
		return fnv;
	}

	/* at the call site, we can't tell if the function is defined as
	 * f(a, b)
	 *   int a, b;   i.e. $f = i4(i4 $a, i4 $b){ ... }
	 * { ... }
	 *
	 * or
	 *
	 * f()           i.e. $f = i4(...){ ... }
	 * { ... }
	 *
	 * so we must assume that we need to cast it to i4(...) to call it
	 */

	casted_val = ctx->curval++;

	printf("\t$%u = ptrcast %s(...)*, %s\n",
			casted_val,
			irtype_str(e->tree_type, ctx),
			irval_str(fnv, ctx));

	return irval_from_id(casted_val);
}

irval *gen_ir_expr_funcall(const expr *e, irctx *ctx)
{
	irval **args = NULL;
	irval *fnv;

	unsigned reti;
	expr **earg;
	irval **irarg;
	const char *comma = "";

	for(earg = e->funcargs; earg && *earg; earg++){
		irval *arg = gen_ir_expr(*earg, ctx);
		dynarray_add(&args, arg);
	}

	/* type check the function expression */
	fnv = funcall_ir_correct_type(e, gen_ir_expr(e->expr, ctx), ctx);

	reti = ctx->curval++;
	printf("\t$%u = call %s(", reti, irval_str(fnv, ctx));

	for(irarg = args; irarg && *irarg; irarg++){
		printf("%s%s", comma, irval_str(*irarg, ctx));

		comma = ", ";
	}

	printf(")\n");

	dynarray_free(irval **, args, irval_free_abi);
	irval_free(fnv);

	return irval_from_id(reti);
}

void dump_expr_funcall(const expr *e, dump *ctx)
{
	expr **iter;

	dump_desc_expr(ctx, "call", e);

	dump_inc(ctx);
	dump_expr(e->expr, ctx);
	dump_dec(ctx);

	dump_inc(ctx);
	for(iter = e->funcargs; iter && *iter; iter++)
		dump_expr(*iter, ctx);

	dump_dec(ctx);
}

void mutate_expr_funcall(expr *e)
{
	(void)e;
}

int expr_func_passable(expr *e)
{
	/* need to check the sub-expr, i.e. the function */
	return !func_attr_present(e, attr_noreturn);
}

expr *expr_new_funcall()
{
	expr *e = expr_new_wrapper(funcall);
	e->freestanding = !cc1_warning.unused_fnret;
	return e;
}

const out_val *gen_expr_style_funcall(const expr *e, out_ctx *octx)
{
	stylef("(");
	IGNORE_PRINTGEN(gen_expr(e->expr, octx));
	stylef(")(");
	if(e->funcargs){
		expr **i;
		for(i = e->funcargs; i && *i; i++){
			IGNORE_PRINTGEN(gen_expr(*i, octx));
			if(i[1])
				stylef(", ");
		}
	}
	stylef(")");

	return NULL;
}
