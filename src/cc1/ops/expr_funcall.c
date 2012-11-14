#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "ops.h"
#include "expr_funcall.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../../util/alloc.h"
#include "../funcargs.h"

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

	type_func = e->tree_type = type_ref_func_call(type_func, &args_from_decl);

	/* func count comparison, only if the func has arg-decls, or the func is f(void) */
	UCC_ASSERT(args_from_decl, "no funcargs for decl %s", e->expr->spel);

	if(e->funcargs){
		int i;

		for(i = 0; e->funcargs[i]; i++){
			expr *arg = FOLD_EXPR(e->funcargs[i], stab);
			char *desc = ustrprintf("function argument to %s", sp);

			fold_need_expr(arg, desc, 0);
			fold_disallow_st_un(arg, desc);

			free(desc);
		}
	}

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

	if(type_attr_present(e->tree_type, attr_format))
		ICW("TODO: format checks on funcall at %s", where_str(&e->where));

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
		sym *const sym = e->expr->sym;
		int nargs = 0;

		if(sym && !type_ref_is_fptr(sym->decl->ref))
			out_push_lbl(sym->decl->spel, 0, NULL);
		else
			gen_expr(e->expr, stab);

		if(e->funcargs){
			const int int_sz = type_primitive_size(type_int);
			expr **aiter;

			for(aiter = e->funcargs; *aiter; aiter++, nargs++);

			for(aiter--; aiter >= e->funcargs; aiter--){
				expr *earg = *aiter;

				gen_expr(earg, stab);

				/* each arg needs casting up to int size, if smaller */
				if(type_ref_size(earg->tree_type, &earg->where) < int_sz)
					out_cast(earg->tree_type, type_ref_new_INT());
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

expr *expr_new_funcall()
{
	expr *e = expr_new_wrapper(funcall);
	e->freestanding = 1;
	return e;
}

void gen_expr_style_funcall(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
