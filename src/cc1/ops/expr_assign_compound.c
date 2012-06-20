#include "ops.h"

const char *str_expr_assign_compound()
{
	return "assign_compound";
}

void fold_expr_assign_compound(expr *e, symtable *stab)
{
	int type_ok;

	fold_inc_writes_if_sym(e->lhs, stab);

	{
		expr *addr = expr_new_addr();

		addr->lhs = e->lhs;

		e->lhs = addr;
		/* take the address of where we're assigning to - only eval once */
	}

	fold_expr(e->lhs, stab);
	fold_expr(e->rhs, stab);

	fold_coerce_assign(e->lhs->tree_type, e->rhs, &type_ok);

	/* skip the addr we inserted */
	if(!expr_is_lvalue(e->lhs->lhs, 0)){
		DIE_AT(&e->lhs->where, "compound target not an lvalue (%s)",
				e->lhs->f_str());
	}

	if(decl_is_const(e->lhs->tree_type))
		DIE_AT(&e->where, "can't modify const expression %s", e->lhs->f_str());

	UCC_ASSERT(op_can_compound(e->op), "non-compound op in compound expr");

	e->tree_type = decl_copy(e->lhs->lhs->tree_type);
}

void gen_expr_assign_compound(expr *e, symtable *stab)
{
	const char *inst, *lhs, *rhs, *pre, *ret;

	/*if(decl_is_struct_or_union(e->tree_type))*/
	fold_disallow_st_un(e, "copy (TODO)"); /* yes this is meant to be in gen */

	lhs = "rax", rhs = "rbx";
	op_get_asm(e->op, &inst, &lhs, &rhs, &pre, &ret);

	gen_expr(e->rhs, stab);
	asm_temp(1, "; saved for compound op");

	gen_expr(e->lhs, stab);

	asm_temp(1, "pop rsi ; compound lval addr");
	asm_temp(1, "mov %s, [rsi]", lhs);
	asm_temp(1, "mov %s, [rsp]", rhs);
	if(e->assign_is_post)
		asm_temp(1, "mov [rsp], %s ; post-inc -> pre value", lhs);

	if(pre)
		asm_temp(1, "%s", pre);

	{
		/* XXX: HACK */
		const int div = e->op == op_divide || e->op == op_modulus;

		if(div){
			asm_temp(1, "%s %s", inst, rhs);
		}else{
			asm_temp(1, "%s %s, %s", inst, lhs, rhs);
		}
	}

	/* store back to the sym's home */
	asm_temp(1, "mov [rsi], %s ; compound store", ret);

	/* need to update the value on the stack */
	if(!e->assign_is_post)
		asm_temp(1, "mov [rsp], %s ; pre-assignment, update stack", lhs);
}

void gen_expr_str_assign_compound(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("compound assignment, expr:\n", e->assign_is_post ? "post-inc/dec " : "");
	idt_printf("assign to:\n");
	gen_str_indent++;
	print_expr(e->lhs->lhs); /* skip our addr */
	gen_str_indent--;
	idt_printf("assign from:\n");
	gen_str_indent++;
	print_expr(e->rhs);
	gen_str_indent--;
}

void mutate_expr_assign_compound(expr *e)
{
	e->freestanding = 1;
}

expr *expr_new_assign_compound(expr *to, expr *from, enum op_type op)
{
	expr *e = expr_new_wrapper(assign_compound);

	e->lhs = to;
	e->rhs = from;
	e->op = op;

	return e;
}

void gen_expr_style_assign_compound(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
