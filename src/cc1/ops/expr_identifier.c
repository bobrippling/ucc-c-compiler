#include <string.h>
#include "ops.h"
#include "../sue.h"

const char *str_expr_identifier()
{
	return "identifier";
}

void fold_const_expr_identifier(expr *e, intval *piv, enum constyness *pconst_type)
{
	(void)piv;
	if(e->sym && e->sym->decl->type->qual == qual_const){
		/*
			* TODO
			* fold. need to hunt for assignment tree
			*/
		//fprintf(stderr, "TODO: fold expression with const identifier %s\n", e->spel);
	}

	*pconst_type = CONST_NO;
}

void fold_expr_identifier(expr *e, symtable *stab)
{
	if(!e->sym){
		if(!strcmp(e->spel, "__func__")){
			/* mutate into a string literal */
			expr_mutate_wrapper(e, addr);

			e->array_store = array_decl_new();

			UCC_ASSERT(curdecl_func, "no spel for current func");
			e->array_store->data.str = curdecl_func->spel;
			e->array_store->len = strlen(curdecl_func->spel) + 1; /* +1 - take the null byte */

			e->array_store->type = array_str;

			fold_expr(e, stab);

		}else{
			/* check for an enum */
			struct_union_enum_st *sue;
			enum_member *m;

			enum_member_search(&m, &sue, stab, e->spel);

			if(!m)
				DIE_AT(&e->where, "undeclared identifier \"%s\"", e->spel);

			expr_mutate_wrapper(e, val);

			e->val = m->val->val;
			fold_expr(e, stab);

			e->tree_type->type->primitive = type_enum;
			e->tree_type->type->sue = sue;
			return;
		}
	}else{
		e->tree_type = decl_copy(e->sym->decl);

		if(e->sym->type == sym_local
		&& !type_store_static_or_extern(e->sym->decl->type->store)
		&& !decl_has_array(e->sym->decl)
		&& !decl_is_struct_or_union(e->sym->decl)
		&& !decl_is_func(e->sym->decl)
		&& e->sym->nwrites == 0)
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

	if(e->sym && !decl_is_func(e->sym->decl)){
		/*
		 * if it's an array, lea, else, load
		 * note that array-leas load the bottom address (smallest value)
		 * since arrays grow upwards... duh
		 *
		 * also never do this for functions
		 */
		asm_sym(decl_has_array(e->sym->decl) ? ASM_LEA : ASM_LOAD, e->sym, "rax");
	}else{
		asm_temp(1, "mov rax, %s", e->spel);
	}

	asm_temp(1, "push rax");
}

void gen_expr_identifier_1(expr *e, FILE *f)
{
	fprintf(f, "%s", e->sym->decl->spel);
	/*
	 * don't use e->spel
	 * static int i;
	 * int x;
	 * x = i; // e->spel is "i". e->sym->decl->spel is "func_name.static_i"
	 */
}

void gen_expr_identifier_store(expr *e, symtable *stab)
{
	(void)stab;
	asm_sym(ASM_SET, e->sym, "rax");
}

void mutate_expr_identifier(expr *e)
{
	e->f_store      = gen_expr_identifier_store;
	e->f_gen_1      = gen_expr_identifier_1;
	e->f_const_fold = fold_const_expr_identifier;
}

expr *expr_new_identifier(char *sp)
{
	expr *e = expr_new_wrapper(identifier);
	e->spel = sp;
	return e;
}

void gen_expr_style_identifier(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
