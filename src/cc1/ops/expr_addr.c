#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "../struct.h"

const char *str_expr_addr()
{
	return "addr";
}

void fold_expr_addr(expr *e, symtable *stab)
{
	if(e->array_store){
		sym *array_sym;

		if(e->sym)
			return; /* already folded */

		UCC_ASSERT(!e->expr, "expression found in array store address-of");

		/* static const char * */
		e->tree_type = decl_new();
		*decl_leaf(e->tree_type) = decl_ptr_new();

		e->tree_type->type->store = store_static;
		e->tree_type->type->qual  = qual_const;

		e->tree_type->type->primitive = type_char;

		e->spel = e->array_store->label = asm_label_array(e->array_store->type == array_str);

		e->tree_type->spel = e->spel;

		array_sym = SYMTAB_ADD(symtab_root(stab), e->tree_type, stab->parent ? sym_local : sym_global);

		array_sym->decl->arrayinit = e->array_store;
		array_sym->decl->internal = 1;


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

		if(expr_kind(e->expr, op) && (e->expr->op == op_struct_dot || e->expr->op == op_struct_ptr)){
			/*
			 * convert &(a.b) to &a + offsetof(a, b)
			 * i.e.:
			 * (__typeof(a.b) *)((void *)(&a) + __offsetof(__typeof(a), b))
			 *
			 * also converts &a->b to:
			 * (__typeof(a->b) *)((void *)a + __offsetof(__typeof(a), b))
			 */
			expr *struc, *member, *addr;
			struct_union_st *st;
			decl *member_decl;

			/* pull out the various bits */
			struc = e->expr->lhs;
			member = e->expr->rhs;
			addr = e->expr;
			st = struc->tree_type->type->struct_union;

			/* forget about the old structure */
			e->expr = e->expr->lhs = e->expr->rhs = NULL;

			/* lookup the member decl (for tree type later on) */
			member_decl = struct_union_member_find(st, member->spel, &e->where);

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

		if(!expr_kind(e->expr, identifier))
			die_at(&e->expr->where, "can't take the address of %s", e->expr->f_str());

		if(e->expr->tree_type->type->store == store_register)
			die_at(&e->expr->where, "can't take the address of register variable %s", e->expr->spel);

		e->tree_type = decl_ptr_depth_inc(decl_copy(e->expr->sym ? e->expr->sym->decl : e->expr->tree_type));
	}
}

void gen_expr_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->array_store){
		/*decl *d = e->array_store->data.exprs[0];*/

		asm_output_new(
				asm_out_type_mov,
				asm_operand_new_reg(  e->tree_type, ASM_REG_A), /* TODO: tt right here? */
				asm_operand_new_label(NULL, e->array_store->label)
			);

	}else{
		/* address of possibly an ident "(&a)->b" or a struct expr "&a->b" */
		if(expr_kind(e->expr, identifier)){
			asm_sym(ASM_LEA, e->expr->sym, asm_operand_new_reg(NULL /* pointer */, ASM_REG_A));
		}else{
			ICE("TODO: address of %s", e->expr->f_str());
		}
	}

	asm_push(ASM_REG_A);
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

void mutate_expr_addr(expr *e)
{
	e->f_gen_1 = gen_expr_addr_1;
}
