#include "ops.h"

const char *str_expr_addr()
{
	return "addr";
}

static int fold_expr_is_addressable(expr *e)
{
	return expr_kind(e, identifier);
}

void fold_expr_addr(expr *e, symtable *stab)
{
	if(e->array_store){
		sym *array_sym;

		UCC_ASSERT(!e->sym, "symbol found when looking for array store");
		UCC_ASSERT(!e->expr, "expression found in array store address-of");

		/* static const char * */
		e->tree_type = decl_new();
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
						die_at(&inits[i]->where, "array init not constant (%s)", inits[i]->f_str());
				}
			}
		}

	}else{
		fold_inc_writes_if_sym(e->expr, stab);

		fold_expr(e->expr, stab);
		if(!fold_expr_is_addressable(e->expr))
			die_at(&e->expr->where, "can't take the address of %s", e->expr->f_str());

		e->tree_type = decl_ptr_depth_inc(decl_copy(e->expr->sym ? e->expr->sym->decl : e->expr->tree_type));
	}
}

void gen_expr_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->array_store){
		asm_temp(1, "mov rax, %s", e->array_store->label);
	}else{
		/* address of possibly an ident "(&a)->b" or a struct expr "&a->b" */
		if(expr_kind(e->expr, identifier)){
			asm_sym(ASM_LEA, e->expr->sym, "rax");
		}else{
			ICE("TODO: address of %s", e->expr->f_str());
		}
	}

	asm_temp(1, "push rax");
}

void gen_expr_addr_1(expr *e, FILE *f)
{
	/* TODO: merge tis code with gen_addr / walk_expr with expr_addr */
	if(e->array_store){
		/* address of an array store */
		fprintf(f, "%s", e->array_store->label);
	}else{
		UCC_ASSERT(expr_kind(e->expr, identifier), "globals addr-of can only be identifier for now");
		fprintf(f, "%s", e->expr->spel);
	}
}

void gen_expr_str_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->array_store){
		if(e->array_store->type == array_str){
			idt_printf("label: %s, \"%s\" (length=%d)\n", e->array_store->label, e->array_store->data.str, e->array_store->len);
		}else{
			int i;
			idt_printf("array: %s:\n", e->array_store->label);
			gen_str_indent++;
			for(i = 0; e->array_store->data.exprs[i]; i++){
				idt_printf("array[%d]:\n", i);
				gen_str_indent++;
				print_expr(e->array_store->data.exprs[i]);
				gen_str_indent--;
			}
			gen_str_indent--;
		}
	}else{
		idt_printf("address of expr:\n");
		gen_str_indent++;
		print_expr(e->expr);
		gen_str_indent--;
	}
}

expr *expr_new_addr()
{
	expr *e = expr_new_wrapper(addr);
	e->f_gen_1 = gen_expr_addr_1;
	return e;
}
