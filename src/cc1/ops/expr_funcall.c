#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_funcall.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../../util/alloc.h"
#include "../funcargs.h"
#include "../data_store.h"

static int func_is_asm(const char *sp)
{
	return fopt_mode & FOPT_ENABLE_ASM
		&& sp
		&& !strcmp(sp, ASM_INLINE_FNAME);
}

const char *str_expr_funcall()
{
	return "funcall";
}

int const_fold_expr_funcall(expr *e)
{
	(void)e;
	return 1; /* could extend to have int x() const; */
}

static void format_check_printf_1(char fmt, type_ref *tt, where *w)
{
	switch(fmt){
		case 's':
			if(!(tt = type_ref_is(tt, type_ref_ptr))
			|| !(tt = type_ref_is_type(tt->ref, type_char)))
			{
				WARN_AT(w, "format %%s expects 'char *' argument");
			}
			break;

		case 'd':
			if(!type_ref_is_integral(tt))
				WARN_AT(w, "format %%d expects integral argument");
			break;

		default:
			WARN_AT(w, "unknown conversion character 0x%x", fmt);
	}
}

static void format_check_printf(
		expr **args,
		unsigned fmt_arg, unsigned var_arg,
		where *w)
{
	expr *e_str;
	type_ref *r;
	intval iv;
	enum constyness k;

	const_fold(args[fmt_arg], &iv, &k);

	if(k != CONST_WITHOUT_VAL){
		WARN_AT(w, "format argument isn't constant");
		return;
	}

	r = type_ref_is(args[fmt_arg]->tree_type, type_ref_ptr);
	if(!r)
		goto wrong_type;
	r = type_ref_is(r->ref, type_ref_type);
	if(!r)
		goto wrong_type;

	if(r->bits.type->primitive != type_char){
wrong_type:
		WARN_AT(w, "format argument isn't a string type");
	}


	e_str = args[fmt_arg];

	if(expr_kind(e_str, cast))
		e_str = e_str->expr;
	if(!expr_kind(e_str, addr)){
		ICW("TODO: format check on non-trivial string expr (%s)",
				args[fmt_arg]->f_str());
		return;
	}

	{
		const char *fmt = e_str->data_store->bits.str;
		const int   len = e_str->data_store->len;
		int i, n_arg = 0;

		for(i = 0; i < len && fmt[i];){
			if(fmt[i++] == '%'){
				expr *e;

				if(fmt[i] == '%'){
					i++;
					continue;
				}

				e = args[var_arg + n_arg++];

				if(!e){
					WARN_AT(w, "too few arguments for format (%%%c)", fmt[i]);
					break;
				}

				format_check_printf_1(fmt[i], e->tree_type, &e->where);
			}
		}
	}
}

static void format_check(where *w, type_ref *ref, expr **args)
{
	decl_attr *attr = type_attr_present(ref, attr_format);
	unsigned n, fmt_arg, var_arg;

	if(!attr)
		return;

	fmt_arg = attr->attr_extra.format.fmt_arg;
	var_arg = attr->attr_extra.format.var_arg;

	n = dynarray_count((void **)args);

	if(fmt_arg >= n)
		DIE_AT(w, "format argument out of bounds (%d >= %d)", fmt_arg, n);
	if(var_arg > n)
		DIE_AT(w, "variadic argument out of bounds (%d >= %d)", var_arg, n);
	if(var_arg <= fmt_arg)
		DIE_AT(w, "variadic argument after format argument");

	switch(attr->attr_extra.format.fmt_func){
		case attr_fmt_printf:
			format_check_printf(args, fmt_arg, var_arg, w);
			break;

		case attr_fmt_scanf:
			ICW("scanf check");
			break;
	}
}

void fold_expr_funcall(expr *e, symtable *stab)
{
	const char *sp = e->expr->spel;
	type_ref *type_func;
	funcargs *args_from_decl;

	if(func_is_asm(sp)){
#if 0
		expr *arg1;
		const char *str;
		int i;

		if(!e->funcargs || e->funcargs[1] || !expr_kind(e->funcargs[0], addr))
			DIE_AT(&e->where, "invalid __asm__ arguments");

		arg1 = e->funcargs[0];
		str = arg1->data_store->data.str;
		for(i = 0; i < arg1->array_store->len - 1; i++){
			char ch = str[i];
			if(!isprint(ch) && !isspace(ch))
invalid:
				DIE_AT(&arg1->where, "invalid __asm__ string (character 0x%x at index %d, %d / %d)",
						ch, i, i + 1, arg1->array_store->len);
		}

		if(str[i])
			goto invalid;

		/* TODO: allow a long return, e.g. __asm__(("movq $5, %rax")) */
		e->tree_type = decl_new_void();
		return;
#else
		ICE("TODO: __asm__");
#endif
	}


	if(!sp)
		sp = "<anon func>";

	if(expr_kind(e->expr, identifier) && e->expr->spel){
		/* check for implicit function */
		if(!(e->expr->sym = symtab_search(stab, sp))){
			funcargs *args = funcargs_new();
			decl *df;

			funcargs_empty(args); /* set up the funcargs as if it's "x()" - i.e. any args */

			type_func = type_ref_new_func(type_ref_new_type(type_new_primitive(type_int)), args);

			cc1_warn_at(&e->where, 0, 1, WARN_IMPLICIT_FUNC, "implicit declaration of function \"%s\"", sp);

			df = decl_new();
			df->ref = type_func;
			df->spel = e->expr->spel;
			df->is_definition = 1; /* needed since it's a local var */

			/* not declared - generate a sym ourselves */
			e->expr->sym = SYMTAB_ADD(stab, df, sym_local);
		}
	}

	FOLD_EXPR(e->expr, stab);
	type_func = e->expr->tree_type;

	if(!type_ref_is_callable(type_func)){
		DIE_AT(&e->expr->where, "%s-expression (type '%s') not callable",
				e->expr->f_str(), type_ref_to_str(type_func));
	}

	if(expr_kind(e->expr, deref)
	&& type_ref_is(type_ref_is(expr_deref_what(e->expr)->tree_type, type_ref_ptr), type_ref_func)){
		/* XXX: memleak */
		/* (*f)() - dereffing to a function, then calling - remove the deref */
		e->expr = expr_deref_what(e->expr);
	}

	e->tree_type = type_ref_func_call(type_func, &args_from_decl);

	/* func count comparison, only if the func has arg-decls, or the func is f(void) */
	UCC_ASSERT(args_from_decl, "no funcargs for decl %s", e->expr->spel);


	/* this block folds the args */
	if(e->funcargs){
		unsigned long nonnulls = 0;
		char *const desc = ustrprintf("function argument to %s", sp);
		const int int_sz = type_primitive_size(type_int);
		type_ref *t_int = type_ref_new_INT();
		int t_int_used = 0;
		int i;

		{
			decl_attr *da;
			if((da = type_attr_present(type_func, attr_nonnull)))
				nonnulls = da->attr_extra.nonnull_args;
		}

		for(i = 0; e->funcargs[i]; i++){
			expr *arg = FOLD_EXPR(e->funcargs[i], stab);

			fold_need_expr(arg, desc, 0);
			fold_disallow_st_un(arg, desc);

			if((nonnulls & (1 << i)) && expr_is_null_ptr(arg))
				WARN_AT(&arg->where, "null passed where non-null required (arg %d)", i + 1);

			/* each arg needs casting up to int size, if smaller */
			if(type_ref_size(arg->tree_type, &arg->where) < int_sz){
				expr *cast = expr_new_cast(t_int, 1);

				cast->expr = arg;
				e->funcargs[i] = cast;
				fold_expr_cast_descend(cast, stab, 0);

				t_int_used = 1;
			}
		}

		if(!t_int_used)
			type_ref_free(t_int);

		free(desc);
	}

	/* this block is purely type checking */
	if(args_from_decl->arglist || args_from_decl->args_void){
		expr **iter_arg;
		decl **iter_decl;
		int count_decl, count_arg;

		count_decl = count_arg = 0;

		for(iter_arg  = e->funcargs;       iter_arg  && *iter_arg;  iter_arg++,  count_arg++);
		for(iter_decl = args_from_decl->arglist; iter_decl && *iter_decl; iter_decl++, count_decl++);

		if(count_decl != count_arg && (args_from_decl->variadic ? count_arg < count_decl : 1)){
			DIE_AT(&e->where, "too %s arguments to function %s (got %d, need %d)",
					count_arg > count_decl ? "many" : "few",
					sp, count_arg, count_decl);
		}

		if(e->funcargs){
			funcargs *args_from_expr = funcargs_new();

			for(iter_arg = e->funcargs; *iter_arg; iter_arg++){
				decl *dtmp = decl_new();
				dtmp->ref = (*iter_arg)->tree_type;

				dynarray_add((void ***)&args_from_expr->arglist, dtmp);
			}

			if(funcargs_equal(args_from_decl, args_from_expr, 0, sp) == funcargs_are_mismatch_count)
				DIE_AT(&e->where, "mismatching argument count to %s", sp);

			funcargs_free(args_from_expr, 1, 0);
		}

		/*funcargs_free(args_from_decl, 1); XXX memleak*/
	}

	fold_disallow_st_un(e, "return");

	format_check(&e->where, e->expr->tree_type, e->funcargs);

	/* check the subexp tree type to get the funcall decl_attrs */
	if(type_attr_present(e->expr->tree_type, attr_warn_unused))
		e->freestanding = 0; /* needs use */
}

void gen_expr_funcall(expr *e, symtable *stab)
{
	const char *const fname = e->expr->spel;

	if(func_is_asm(fname)){
		out_comment("start manual __asm__");
		ICE("same");
#if 0
		fprintf(cc_out[SECTION_TEXT], "%s\n", e->funcargs[0]->data_store->data.str);
#endif
		out_comment("end manual __asm__");
	}else{
		/* continue with normal funcall */
		int nargs = 0;

		gen_expr(e->expr, stab);

		if(e->funcargs){
			expr **aiter;

			for(aiter = e->funcargs; *aiter; aiter++, nargs++);

			for(aiter--; aiter >= e->funcargs; aiter--){
				expr *earg = *aiter;

				/* should be of size int or larger (for integral types)
				 * or double (for floating types)
				 */
				gen_expr(earg, stab);
			}
		}

		out_call(nargs, e->tree_type, e->expr->tree_type);
	}
}

void gen_expr_str_funcall(expr *e, symtable *stab)
{
	expr **iter;

	(void)stab;

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
	return !type_attr_present(e->expr->tree_type, attr_noreturn);
}

expr *expr_new_funcall()
{
	expr *e = expr_new_wrapper(funcall);
	e->freestanding = 1;
	return e;
}

void gen_expr_style_funcall(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
