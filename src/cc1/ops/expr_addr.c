#include <string.h>
#include <stdlib.h>

#include "ops.h"
#include "../sue.h"
#include "../str.h"
#include "../data_store.h"
#include "../../util/dynarray.h"

const char *str_expr_addr()
{
	return "addr";
}

void fold_expr_addr(expr *e, symtable *stab)
{
	if(e->data_store){
		data_store_fold_decl(e->data_store, &e->tree_type);
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

		/* lvalues are identifier, struct-exp or deref */
		if(!expr_is_lvalue(e->lhs, 1))
			DIE_AT(&e->lhs->where, "can't take the address of %s", e->lhs->f_str());


		if(e->lhs->tree_type->type->store == store_register)
			DIE_AT(&e->lhs->where, "can't take the address of register variable %s", e->lhs->spel);

		e->tree_type = decl_ptr_depth_inc(decl_copy(e->lhs->sym ? e->lhs->sym->decl : e->lhs->tree_type));
	}
}

void gen_expr_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->data_store){
		asm_temp(1, "mov rax, %s", e->data_store->spel);

		data_store_declare(e->data_store, cc_out[SECTION_DATA]);
		data_store_out(    e->data_store, cc_out[SECTION_DATA]);
	}else{
		/* address of possibly an ident "(&a)->b" or a struct expr "&a->b" */
		if(expr_kind(e->lhs, identifier)){
			asm_sym(ASM_LEA, e->lhs->sym, "rax");
		}else if(expr_kind(e->lhs, op)){
			/* skip the address (e->lhs) and the deref */
			gen_expr(op_deref_expr(e->lhs), stab);
		}else{
			ICE("address of %s", e->lhs->f_str());
		}
	}

	asm_temp(1, "push rax");
}

void gen_expr_addr_1(expr *e, FILE *f)
{
	if(e->data_store){
		/*fprintf(f, "%s", e->data_store->spel);*/
		data_store_declare(e->data_store, f);
		data_store_out(    e->data_store, f);
	}else{
		UCC_ASSERT(expr_kind(e->lhs, identifier), "globals addr-of can only be identifier for now");
		fprintf(f, "%s", e->lhs->spel);
	}
}

void gen_expr_str_addr(expr *e, symtable *stab)
{
	(void)stab;

	if(e->data_store){
		/*asm_temp(1, "mov rax, %s", e->data_store->sym_ident);*/
		idt_printf("address of datastore %s\n", e->data_store->spel);
		gen_str_indent++;
		idt_print();
		literal_print(cc1_out, e->data_store->bits.str, e->data_store->len);
		gen_str_indent--;
		fputc('\n', cc1_out);
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

expr *expr_new_addr_data(data_store *ds)
{
	expr *e = expr_new_wrapper(addr);
	e->data_store = ds;
	return e;
}
