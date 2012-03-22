int exec_op_deref(op)
{
	/*
	 * potential for major ICE here
	 * I mean, we might not even be dereferencing the right size pointer
	 */
	/*
	switch(lhs->vartype->primitive){
		case type_int:  return *(int *)lhs->val.s;
		case type_char: return *       lhs->val.s;
		default:
			break;
	}

	ignore for now, just deal with simple stuff
	*/
	if(lhs->ptr_safe && expr_kind(lhs, addr)){
		if(lhs->array_store->type == array_str)
			return *lhs->val.s;
		/*return lhs->val.exprs[0]->val.i;*/
	}
	*bad = 1;
	return 0;
}

void fold_deref(expr *e)
{
	/* check for *&x */
	if(expr_kind(e->lhs, addr))
		warn_at(&e->lhs->where, "possible optimisation for *& expression");

	e->tree_type = decl_ptr_depth_dec(decl_copy(e->lhs->tree_type));

	if(decl_ptr_depth(e->tree_type) == 0 && e->lhs->tree_type->type->primitive == type_void)
		die_at(&e->where, "can't dereference void pointer");
}
