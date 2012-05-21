#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "../sue.h"
#include "../str.h"
#include "../namespace.h"

const char *str_expr_addr()
{
	return "addr";
}

void fold_expr_addr(expr *e, symtable *stab)
{
#define TT e->tree_type

	if(e->array_store){
		sym *array_sym;

		UCC_ASSERT(!e->sym, "symbol found when looking for array store");
		UCC_ASSERT(!e->lhs, "expression found in array store address-of");

		/* static const char [] */
		TT = decl_new();

		TT->desc = decl_desc_array_new(e->tree_type, NULL);
		TT->desc->bits.array_size = expr_new_val(e->array_store->len);

		TT->type->store = store_static;
		TT->type->qual  = qual_const;

		e->array_store->label = asm_label_array(e->array_store->type == array_str);

		decl_set_spel(e->tree_type, namespace_spel(e->ns_path));

		array_sym = SYMTAB_ADD(symtab_root(stab), e->tree_type, stab->parent ? sym_local : sym_global);

		array_sym->decl->arrayinit = e->array_store;
		array_sym->decl->internal = 1;


		switch(e->array_store->type){
			case array_str:
				e->tree_type->type->primitive = type_char;
				TT->type->primitive = type_char;
				break;

			case array_exprs:
			{
				expr **inits;
				int i;

				TT->type->primitive = type_int;

				inits = e->array_store->data.exprs;

				for(i = 0; inits[i]; i++){
					fold_expr(inits[i], stab);
					if(!const_expr_is_const(inits[i]))
						die_at(&inits[i]->where, "array init not constant (%s)", inits[i]->f_str());
				}
			}
		}

	}else{
		fold_inc_writes_if_sym(e->lhs, stab);

		fold_expr(e->lhs, stab);

		if(expr_kind(e->lhs, op) && (e->lhs->op == op_struct_dot || e->lhs->op == op_struct_ptr)){
			/*
			 * convert &(a.b) to &a + offsetof(a, b)
			 * i.e.:
			 * (__typeof(a.b) *)((void *)(&a) + __offsetof(__typeof(a), b))
			 *
			 * also converts &a->b to:
			 * (__typeof(a->b) *)((void *)a + __offsetof(__typeof(a), b))
			 */
			expr *struc, *member, *addr;
			struct_union_enum_st *st;
			decl *member_decl;

			/* pull out the various bits */
			struc = e->lhs->lhs;
			member = e->lhs->rhs;
			addr = e->lhs;
			st = struc->tree_type->type->sue;

			/* forget about the old structure */
			e->lhs = e->lhs->lhs = e->lhs->rhs = NULL;

			/* lookup the member decl (for tree type later on) */
			member_decl = struct_union_member_find(st, member->ns_path, &e->where);

			/* e is now the op */
			expr_mutate_wrapper(e, op);
			e->op = op_plus;
			e->op_no_ptr_mul = 1;

			/* add the struct addr and the member offset */
			e->lhs = struc;
			e->rhs = expr_new_val(member_decl->struct_offset);

			/* sort yourself out */
			fold_expr(e, stab);

			/* replace the decl */
			decl_free(e->tree_type);
			e->tree_type = decl_ptr_depth_inc(decl_copy(member_decl));

			expr_free(addr);
			return;
		}

		/* lvalues are identifier, struct-exp or deref */
		if(!expr_is_lvalue(e->lhs, 1))
			die_at(&e->lhs->where, "can't take the address of %s", e->lhs->f_str());


		if(e->lhs->tree_type->type->store == store_register)
			die_at(&e->lhs->where, "can't take the address of register variable %s",
					namespace_spel(e->lhs->spel));

		e->tree_type = decl_ptr_depth_inc(decl_copy(e->lhs->sym ? e->lhs->sym->decl : e->lhs->tree_type));
	}
}

void gen_expr_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->array_store){
		asm_temp(1, "mov rax, %s", e->array_store->label);
	}else{
		/* address of possibly an ident "(&a)->b" or a struct expr "&a->b" */
		if(expr_kind(e->lhs, identifier)){
			asm_sym(ASM_LEA, e->lhs->sym, "rax");
		}else if(expr_kind(e->lhs, op)){
			/* skip the address (e->lhs) and the deref */
			gen_expr(op_deref_expr(e->lhs), stab);
		}else{
			ICE("TODO: address of %s", e->lhs->f_str());
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
		UCC_ASSERT(expr_kind(e->lhs, identifier), "globals addr-of can only be identifier for now");
		fprintf(f, "%s", e->lhs->spel);
	}
}

void gen_expr_str_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->array_store){
		if(e->array_store->type == array_str){
			idt_printf("label: %s, \"", e->array_store->label);
			literal_print(cc1_out, e->array_store->data.str, e->array_store->len);
			fprintf(cc1_out, "\" (length=%d)\n", e->array_store->len);
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
		print_expr(e->lhs);
		gen_str_indent--;
	}
}

void mutate_expr_addr(expr *e)
{
	e->f_gen_1 = gen_expr_addr_1;
}

void gen_expr_style_addr(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
