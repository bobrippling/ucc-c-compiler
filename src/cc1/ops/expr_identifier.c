#include <string.h>
#include "ops.h"
#include "../enum.h"

const char *str_expr_identifier()
{
	return "identifier";
}

int fold_const_expr_identifier(expr *e)
{
	if(e->sym && e->sym->decl->type->spec & spec_const){
		/*
			* TODO
			* fold. need to hunt for assignment tree
			*/
		//fprintf(stderr, "TODO: fold expression with const identifier %s\n", e->spel);
	}
	return 1;
}

void fold_expr_identifier(expr *e, symtable *stab)
{
	if(!e->sym){
		if(!strcmp(e->spel, "__func__")){
			/* mutate into a string literal */
			expr_mutate_wrapper(e, addr);

			e->array_store = array_decl_new();

			UCC_ASSERT(curdecl_func_sp, "no spel for current func");
			e->array_store->data.str = curdecl_func_sp;
			e->array_store->len = strlen(curdecl_func_sp) + 1; /* +1 - take the null byte */

			e->array_store->type = array_str;

			fold_expr(e, stab);

		}else{
			/* check for an enum */
			enum_member *m = enum_find_member(stab, e->spel);

			if(!m)
				die_at(&e->where, "undeclared identifier \"%s\"", e->spel);

			expr_mutate_wrapper(e, val);

			e->val = m->val->val;
			fold_expr(e, stab);

			return;
		}
	}else{
		e->tree_type = decl_copy(e->sym->decl);

		if(e->sym->type == sym_local
		&& (e->sym->decl->type->spec & (spec_extern | spec_static)) == 0
		&& !decl_has_array(e->sym->decl)
		&& e->sym->nwrites == 0)
		{
			cc1_warn_at(&e->where, 0, WARN_READ_BEFORE_WRITE, "\"%s\" uninitialised on read", e->sym->decl->spel);
		}

		e->sym->nreads++;
	}
}

void gen_expr_str_identifier(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("identifier: \"%s\" (sym %p)\n", e->spel, e->sym);
}

void gen_expr_identifier_1(expr *e, FILE *f)
{
	asm_out_intval(f, &e->val.iv);
}

void gen_expr_identifier(expr *e, symtable *stab)
{
	(void)stab;

	if(e->sym){
		/*
			* if it's an array, lea, else, load
			* note that array-leas load the bottom address (smallest value)
			* since arrays grow upwards... duh
			*/
		asm_sym(decl_has_array(e->sym->decl) ? ASM_LEA : ASM_LOAD, e->sym, "rax");
	}else{
		asm_temp(1, "mov rax, %s", e->spel);
	}

	asm_temp(1, "push rax");
}

void gen_expr_identifier_store(expr *e, symtable *stab)
{
	(void)stab;
	asm_sym(ASM_SET, e->sym, "rax");
}

expr *expr_new_identifier(char *sp)
{
	expr *e = expr_new_wrapper(identifier);
	e->spel = sp;

	e->f_store      = gen_expr_identifier_store;
	e->f_gen_1      = gen_expr_identifier_1;
	e->f_const_fold = fold_const_expr_identifier;

	return e;
}
