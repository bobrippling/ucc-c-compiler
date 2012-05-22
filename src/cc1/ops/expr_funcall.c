#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "ops.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../../util/alloc.h"

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
		/* check for implicit function */
		char *const sp = e->expr->spel;

		if(!(e->expr->sym = symtab_search(stab, sp))){
			df = decl_new();

			df->type->primitive = type_int;
			df->type->store     = store_extern;

			cc1_warn_at(&e->where, 0, WARN_IMPLICIT_FUNC, "implicit declaration of function \"%s\"", sp);

			decl_set_spel(df, sp);

			df->desc = decl_desc_func_new(df, NULL);
			df->desc->bits.func = funcargs_new();

			/* set up the funcargs as if it's "x()" - i.e. any args */
			function_empty_args(df->desc->bits.func);

			/* not declared - generate a sym ourselves */
			e->expr->sym = SYMTAB_ADD(stab, df, sym_local);
		}
	}

	fold_expr(e->expr, stab);
	df = e->expr->tree_type;

	if(!decl_is_callable(df)){
		die_at(&e->expr->where, "expression %s (%s) not callable",
				e->expr->f_str(),
				decl_to_str(df));
	}

	if(expr_kind(e->expr, op)
	&& e->expr->op == op_deref
	&& decl_is_fptr(op_deref_expr(e->expr)->tree_type)){
		/* XXX: memleak */
		/* (*f)() - dereffing to a function, then calling - remove the deref */
		e->expr = op_deref_expr(e->expr);
	}

	df = e->tree_type = decl_func_deref(decl_copy(df), &args_exp);

	/* func count comparison, only if the func has arg-decls, or the func is f(void) */
	UCC_ASSERT(args_exp, "no funcargs for decl %s", decl_spel(df));

	if(e->funcargs){
		expr **iter;
		char *sp = decl_spel(df);

		if(!sp)
			sp = "<anon func>";

		for(iter = e->funcargs; *iter; iter++){
			char *desc;
			expr *arg = *iter;

			fold_expr(arg, stab);

			desc = umalloc(strlen(sp) + 25);
			sprintf(desc, "function argument to %s", sp);

			fold_disallow_st_un(arg, desc);

			free(desc);
		}
	}

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
					decl_spel(df), count_arg, count_decl);
		}

		if(e->funcargs){
			funcargs *argument_decls = funcargs_new();

			for(iter_arg = e->funcargs; *iter_arg; iter_arg++)
				dynarray_add((void ***)&argument_decls->arglist, (*iter_arg)->tree_type);

			if(!funcargs_equal(args_exp, argument_decls, 0))
				die_at(&e->where, "mismatching argument to function");

			funcargs_free(argument_decls, 0);
		}

		/*funcargs_free(args_exp, 1); XXX memleak*/
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
				die_at(&arg1->where, "invalid __asm__ string (character 0x%x at index %d, %d / %d)",
						ch, i, i + 1, arg1->array_store->len);
		}

		if(str[i])
			goto invalid;

		asm_temp(0, "; start manual __asm__");
		fprintf(cc_out[SECTION_TEXT], "%s\n", arg1->array_store->data.str);
		asm_temp(0, "; end manual __asm__");
	}else{
		/* continue with normal funcall */
		sym *const sym = e->expr->sym;

		if(e->funcargs){
			/* need to push on in reverse order */
			for(iter = e->funcargs; *iter; iter++);
			for(iter--; iter >= e->funcargs; iter--){
				gen_expr(*iter, stab);
				nargs++;
			}
		}

		if(sym && !decl_is_fptr(sym->decl)){
			/* simple */
			asm_temp(1, "call %s", decl_spel(sym->decl));
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
