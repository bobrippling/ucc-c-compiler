int exec_op_struct_ptr(op)
{
	*bad = 1;
}

void fold_op_struct(expr *e, symtable *stab)
{
	/*
	 * lhs = any ptr-to-struct expr
	 * rhs = struct member ident
	 */
	const int ptr_depth_exp = e->op == op_struct_ptr ? 1 : 0;
	struct_st *st;
	decl *d, **i;
	char *spel;

	fold_expr(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	if(!expr_kind(e->rhs, identifier))
		die_at(&e->rhs->where, "struct member must be an identifier (got %s)", e->rhs->f_str());
	spel = e->rhs->spel;

	/* we access a struct, of the right ptr depth */
	if(e->lhs->tree_type->type->primitive != type_struct
			|| decl_ptr_depth(e->lhs->tree_type) != ptr_depth_exp)
		die_at(&e->lhs->where, "%s is not a %sstruct",
				decl_to_str(e->lhs->tree_type),
				ptr_depth_exp == 1 ? "pointer-to-" : "");

	st = e->lhs->tree_type->type->struc;

	if(!st)
		die_at(&e->lhs->where, "%s incomplete type",
				ptr_depth_exp == 1 /* this should always be true.. */
				? "dereferencing pointer to"
				: "use of");

	/* found the struct, find the member */
	d = NULL;
	for(i = st->members; i && *i; i++)
		if(!strcmp((*i)->spel, spel)){
			d = *i;
			break;
		}

	if(!d)
		die_at(&e->rhs->where, "struct %s has no member named \"%s\"", st->spel, spel);

	e->rhs->tree_type = d;

	/*
	 * if it's a.b, convert to (&a)->b for asm gen
	 * e = { lhs = "a", rhs = "b", type = dot }
	 * e = { lhs = { expr = "a", type = addr }, rhs = "b", type = ptr }
	 */
	if(ptr_depth_exp == 0){
		expr *new = expr_new_addr();

		new->expr = e->lhs;
		e->lhs = new;

		expr_mutate_wrapper(e, op);
		e->op   = op_struct_ptr;

		fold_expr(e->lhs, stab);
		e->tree_type = decl_copy(e->lhs->tree_type);
	}else{
		e->tree_type = decl_copy(d);
	}
}

void asm_operate_struct(expr *e, symtable *tab)
{
	(void)tab;

	UCC_ASSERT(e->op == op_struct_ptr, "a.b should have been handled by now");

	gen_expr(e->lhs, tab);

	/* pointer to the struct is on the stack, get from the offset */
	asm_temp(1, "pop rax ; struct ptr");
	asm_temp(1, "add rax, %d ; offset of member %s",
			e->rhs->tree_type->struct_offset,
			e->rhs->spel);
	asm_temp(1, "mov rax, [rax] ; val from struct");
	asm_temp(1, "push rax");
}

