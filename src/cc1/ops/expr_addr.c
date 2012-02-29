#include "../data_structs.h"
#include "expr_sizeof.h"

static int fold_expr_is_addressable(expr *e)
{
	return e->type == expr_identifier;
}

expr *expr_new_addr()
{
	e->array_store = array_decl_new();
}

void fold_expr_assign(expr *e, symtable *stab)
{
	if(e->array_store){
		sym *array_sym;

		UCC_ASSERT(!e->sym, "symbol found when looking for array store");
		UCC_ASSERT(!e->expr, "expression found in array store address-of");

		/* static const char * */
		*decl_leaf(e->tree_type) = decl_ptr_new();

		e->tree_type->type->spec |= spec_static | spec_const;
		e->tree_type->type->primitive = type_char;

		e->spel = e->array_store->label = asm_label_array(e->array_store->type == array_str);

		e->tree_type->spel = e->spel;

		array_sym = SYMTAB_ADD(symtab_root(stab), e->tree_type, stab->parent ? sym_local : sym_global);

		array_sym->decl->arrayinit = e->array_store;


		switch(e->array_store->type){
			case array_str:
				e->tree_type->type->primitive = type_char;
				break;

			case array_exprs:
			{
				expr **inits;
				int i;

				e->tree_type->type->primitive = type_int;

				inits = e->array_store->data.exprs;

				for(i = 0; inits[i]; i++){
					fold_expr(inits[i], stab);
					if(const_fold(inits[i]))
						die_at(&inits[i]->where, "array init not constant (%s)", expr_to_str(inits[i]->type));
				}
			}
		}

	}else{
		fold_expr(e->expr, stab);
		if(!fold_expr_is_addressable(e->expr))
			die_at(&e->expr->where, "can't take the address of %s", expr_to_str(e->expr->type));

		GET_TREE_TYPE(e->expr->sym ? e->expr->sym->decl : e->expr->tree_type);

		e->tree_type = decl_ptr_depth_inc(e->tree_type);
	}
}

void gen_expr_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->array_store){
		asm_temp(1, "mov rax, %s", e->array_store->label);
	}else{
		/* address of possibly an ident "(&a)->b" or a struct expr "&a->b" */
		if(e->expr->type == expr_identifier){
			asm_sym(ASM_LEA, e->expr->sym, "rax");
		}else{
			ICE("TODO: address of %s", expr_to_str(e->expr->type));
		}
	}

	asm_temp(1, "push rax");
}
