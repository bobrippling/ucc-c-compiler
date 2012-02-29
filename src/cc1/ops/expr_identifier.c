#include "../data_structs.h"
#include "expr_sizeof.h"

void fold_const_expr_identifier(expr *e)
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
}

void gen_expr_identifier(expr *e, symtable *stab)
{
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
