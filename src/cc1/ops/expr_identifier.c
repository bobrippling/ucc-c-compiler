#include <string.h>
#include "ops.h"
#include "expr_identifier.h"
#include "../out/asm.h"
#include "../sue.h"
#include "expr_addr.h"
#include "../data_store.h"

const char *str_expr_identifier()
{
	return "identifier";
}

void fold_const_expr_identifier(expr *e, intval *piv, enum constyness *pconst_type)
{
	(void)piv;

	/*
	 * if we are an array identifier, we are constant:
	 * int x[];
	 */

	/* may not have e->sym if we're the struct-member-identifier */

	*pconst_type = e->sym && e->sym->decl && decl_is_array(e->sym->decl) ? CONST_WITHOUT_VAL : CONST_NO;
}

void fold_expr_identifier(expr *e, symtable *stab)
{
	if(!e->sym){
		if(!strcmp(e->spel, "__func__")){
			char *sp;
			int len;

			/* mutate into a string literal */
			expr_mutate_wrapper(e, addr);

			if(!curdecl_func){
				WARN_AT(&e->where, "__func__ is not defined outside of functions");
				sp = "";
				len = 0;
			}else{
				sp = curdecl_func->spel;
				len = strlen(curdecl_func->spel);
			}

			expr_mutate_addr_data(e, sp, len + 1);
			/* +1 - take the null byte */

			FOLD_EXPR(e, stab);
		}else{
			/* check for an enum */
			struct_union_enum_st *sue;
			enum_member *m;

			enum_member_search(&m, &sue, stab, e->spel);

			if(!m)
				DIE_AT(&e->where, "undeclared identifier \"%s\"", e->spel);

			expr_mutate_wrapper(e, val);

			e->val = m->val->val;
			/*FOLD_EXPR(e, stab);*/

			{
				type *t;
				e->tree_type = type_ref_new_type(t = type_new_primitive(type_enum));
				t->sue = sue;
			}
		}
	}else{
		e->tree_type = e->sym->decl;

#if 0
Except when it is the operand of the sizeof operator or the unary
& operator, or is a string literal used to initialize an array, an
expression that has type `array of type` is converted to an expression
with type `pointer to type` that points to the initial element of the
array object and is not an lvalue.
#endif

		if(e->sym->type == sym_local
		&& !type_store_static_or_extern(e->sym->decl->store)
		&& !decl_has_array(e->sym->decl)
		&& !decl_is_struct_or_union_possible_ptr(e->sym->decl)
		&& !decl_is_func(e->sym->decl)
		&& e->sym->nwrites == 0
		&& !e->sym->decl->init)
		{
			cc1_warn_at(&e->where, 0, 1, WARN_READ_BEFORE_WRITE, "\"%s\" uninitialised on read", e->spel);
			e->sym->nwrites = 1; /* silence future warnings */
		}

		/* this is cancelled by expr_assign in the case we fold for an assignment to us */
		e->sym->nreads++;
	}
}

void gen_expr_str_identifier(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("identifier: \"%s\" (sym %p)\n", e->spel, e->sym);
}

void gen_expr_identifier(expr *e, symtable *stab)
{
	(void)stab;

	if(decl_is_func(e->sym->decl))
		out_push_sym(e->sym);
	else
		out_push_sym_val(e->sym);
}

void static_expr_identifier_store(expr *e)
{
	asm_declare_partial("%s", e->sym->decl->spel);
	/*
	 * don't use e->spel
	 * static int i;
	 * int x;
	 * x = i; // e->spel is "i". e->sym->decl->spel is "func_name.static_i"
	 */
}

void gen_expr_identifier_lea(expr *e, symtable *stab)
{
	(void)stab;

	out_push_sym(e->sym);
}

void mutate_expr_identifier(expr *e)
{
	e->f_lea         = gen_expr_identifier_lea;
	e->f_static_addr = static_expr_identifier_store;
	e->f_const_fold  = fold_const_expr_identifier;
}

expr *expr_new_identifier(char *sp)
{
	expr *e = expr_new_wrapper(identifier);
	e->spel = sp;
	return e;
}

void gen_expr_style_identifier(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
