#include <string.h>
#include <ctype.h>
#include "ops.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"

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
	decl *df;
	funcargs *args_exp;

	if(expr_kind(e->expr, identifier) && e->expr->spel){
		char *const sp = e->expr->spel;

		e->sym = symtab_search(stab, sp);
		if(!e->sym){
			df = decl_new();

			df->type->primitive = type_int;
			df->type->store     = store_extern;

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

		/*
		 * convert int (*)() to remove the deref
		 */
		if(decl_is_func_ptr(e->expr->tree_type)){
			/* XXX: memleak */
			e->expr = e->expr->lhs;
			fprintf(stderr, "FUNCPTR\n");
		}else{
			fprintf(stderr, "decl %s\n", decl_to_str(e->expr->tree_type));
		}

		df = e->expr->tree_type;

		if(!decl_is_callable(df)){
			die_at(&e->expr->where, "expression %s (%s) not callable",
					e->expr->f_str(),
					decl_to_str(df));
		}
	}

	e->tree_type = decl_copy(df);
	/*
	 * int (*x)();
	 * (*x)();
	 * evaluates to tree_type = int;
	 */
	decl_func_deref(e->tree_type);


	if(e->funcargs){
		expr **iter;
		for(iter = e->funcargs; *iter; iter++){
			expr *arg = *iter;

			fold_expr(arg, stab);

			fold_disallow_st_un(arg, "function argument");
		}
	}

	/* func count comparison, only if the func has arg-decls, or the func is f(void) */
	args_exp = decl_funcargs(e->tree_type);

	UCC_ASSERT(args_exp, "no funcargs for decl %s", df->spel);

	if(args_exp->arglist || args_exp->args_void){
		expr **iter_arg;
		decl **iter_decl;
		int count_decl, count_arg;

		count_decl = count_arg = 0;

		for(iter_arg  = e->funcargs;       iter_arg  && *iter_arg;  iter_arg++,  count_arg++);
		for(iter_decl = args_exp->arglist; iter_decl && *iter_decl; iter_decl++, count_decl++);

		if(count_decl != count_arg && (args_exp->variadic ? count_arg < count_decl : 1)){
			die_at(&e->where, "too %s arguments to function %s (got %d, need %d)",
					count_arg > count_decl ? "many" : "few",
					df->spel, count_arg, count_decl);
		}

		if(e->funcargs){
			funcargs *argument_decls = funcargs_new();

			for(iter_arg = e->funcargs; *iter_arg; iter_arg++)
				dynarray_add((void ***)&argument_decls->arglist, (*iter_arg)->tree_type);

			fold_funcargs_equal(args_exp, argument_decls, 1, &e->where, "argument", df->spel);
			funcargs_free(argument_decls, 0);
		}
	}

	fold_disallow_st_un(e, "return");

	if(decl_attr_present(e->tree_type->attr, attr_format))
		ICW("TODO: format checks on funcall at %s", where_str(&e->where));

	if(decl_attr_present(e->tree_type->attr, attr_warn_unused))
		e->freestanding = 0; /* needs use */
}

void gen_expr_funcall(expr *e, symtable *stab)
{
	const char *const fname = e->expr->spel;
	expr **iter;
	int nargs = 0;

	if(fopt_mode & FOPT_ENABLE_ASM && fname && !strcmp(fname, ASM_INLINE_FNAME)){
		const char *str;
		expr *arg1;
		int i;

		if(!e->funcargs || e->funcargs[1] || !expr_kind(e->funcargs[0], addr))
			die_at(&e->where, "invalid __asm__ arguments");

		arg1 = e->funcargs[0];
		str = arg1->array_store->data.str;
		for(i = 0; i < arg1->array_store->len - 1; i++){
			char ch = str[i];
			if(!isprint(ch) && !isspace(ch))
invalid:
				die_at(&arg1->where, "invalid __asm__ string (character %d)", ch);
		}

		if(str[i])
			goto invalid;

		asm_temp(0, "; start manual __asm__");
		fprintf(cc_out[SECTION_TEXT], "%s\n", arg1->array_store->data.str);
		asm_temp(0, "; end manual __asm__");
	}else{
		/* continue with normal funcall */

		if(e->funcargs){
			/* need to push on in reverse order */
			for(iter = e->funcargs; *iter; iter++);
			for(iter--; iter >= e->funcargs; iter--){
				gen_expr(*iter, stab);
				nargs++;
			}
		}

		if(e->sym && !e->sym->decl->decl_ptr && e->sym->decl->spel){
			/* simple */
			asm_temp(1, "call %s", e->sym->decl->spel);
		}else{
			gen_expr(e->expr, stab);
			asm_temp(1, "pop rax  ; function address");
			asm_temp(1, "call rax ; duh");
		}

		if(nargs)
			asm_temp(1, "add rsp, %d ; %d arg%s",
					nargs * platform_word_size(),
					nargs,
					nargs == 1 ? "" : "s");

		asm_temp(1, "push rax ; ret");
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
