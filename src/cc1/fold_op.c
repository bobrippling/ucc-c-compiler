void fold_op_struct(expr *e, symtable *stab)
{
	/*
	 * lhs = any ptr-to-struct expr
	 * rhs = struct member ident
	 */
	const int ptr_depth_exp = e->op == op_struct_ptr ? 1 : 0;
	struc *st;
	decl *d, **i;
	char *spel;

	fold_expr(e->lhs, stab);
	/* don't fold the rhs - just a member name */

	if(e->rhs->type != expr_identifier)
		die_at(&e->rhs->where, "struct member must be an identifier");
	spel = e->rhs->spel;

	/* we either access a struct or an identifier */
	if(e->lhs->tree_type->type->primitive != type_struct || e->lhs->tree_type->ptr_depth != ptr_depth_exp)
		die_at(&e->lhs->where, "%s is not a %sstruct", decl_to_str(e->lhs->tree_type), ptr_depth_exp == 1 ? "pointer-to-" : "");

	st = e->lhs->tree_type->type->struc;

	UCC_ASSERT(st, "NULL ->struc in tree_type");

	/* found the struct, find the member */
	d = NULL;
	for(i = st->members; *i; i++)
		if(!strcmp((*i)->spel, spel)){
			d = *i;
			break;
		}

	if(!d)
		die_at(&e->rhs->where, "struct %s has no member named \"%s\"", st->spel, spel);

	GET_TREE_TYPE_TO(e->rhs, d);
	GET_TREE_TYPE(d);
}

void fold_op(expr *e, symtable *stab)
{
	if(e->op == op_struct_ptr || e->op == op_struct_dot){
		fold_op_struct(e, stab);
		return;
	}else{
		fold_expr(e->lhs, stab);
		if(e->rhs)
			fold_expr(e->rhs, stab);
	}

	if(e->rhs){
		enum {
			SIGNED, UNSIGNED
		} rhs, lhs;

		rhs = e->rhs->tree_type->type->spec & spec_unsigned ? UNSIGNED : SIGNED;
		lhs = e->lhs->tree_type->type->spec & spec_unsigned ? UNSIGNED : SIGNED;

		if(rhs != lhs){
#define SIGN_CONVERT(test_hs, assert_hs) \
			if(e->test_hs->type == expr_val && e->test_hs->val.i >= 0){ \
				/*                                              \
					* assert(lhs == UNSIGNED);                     \
					* vals default to signed, change to unsigned   \
					*/                                             \
				UCC_ASSERT(assert_hs == UNSIGNED,               \
						"signed-unsigned assumption failure");      \
																												\
				e->test_hs->tree_type->type->spec |= spec_unsigned; \
				goto noproblem;                                 \
			}

			SIGN_CONVERT(rhs, lhs)
			SIGN_CONVERT(lhs, rhs)

			cc1_warn_at(&e->where, 0, WARN_SIGN_COMPARE, "comparison between signed and unsigned");
		}
	}
noproblem:

	/* XXX: note, this assumes that e.g. "1 + 2" the lhs and rhs have the same type */
	if(e->op == op_deref){
		/* check for *&x */

		if(e->lhs->type == expr_addr)
			warn_at(&e->lhs->where, "possible optimisation for *& expression");


		GET_TREE_TYPE(e->lhs->tree_type);

		/*
		 * ensure ->arraysizes is kept in sync
		 */
		e->tree_type->ptr_depth--;
		memmove(
				&e->tree_type->arraysizes[0],
				&e->tree_type->arraysizes[1],
				dynarray_count((void **)e->tree_type->arraysizes)
				);

		if(e->tree_type->ptr_depth == 0)
			switch(e->lhs->tree_type->type->primitive){
				case type_unknown:
				case type_void:
					die_at(&e->where, "can't dereference void pointer");
				default:
					/* e->tree_type already set to deref type */
					break;
			}
		else if(e->tree_type->ptr_depth < 0)
			die_at(&e->where, "can't dereference non-pointer (%s)", type_to_str(e->tree_type->type));
	}else{
		/* look either side - if either is a pointer, take that as the tree_type */
		/* TODO: checks for pointer + pointer (invalid), etc etc */
		if(e->rhs && e->rhs->tree_type->ptr_depth)
			GET_TREE_TYPE(e->rhs->tree_type);
		else
			GET_TREE_TYPE(e->lhs->tree_type);
	}

	if(e->rhs){
		/* need to do this check _after_ we get the correct tree type */
		if((e->op == op_plus || e->op == op_minus) &&
				e->tree_type->ptr_depth &&
				e->rhs){

			/* 2 + (void *)5 is 7, not 2 + 8*5 */
			if(e->tree_type->type->primitive != type_void){
				/* we're dealing with pointers, adjust the amount we add by */

				if(e->lhs->tree_type->ptr_depth)
					/* lhs is the pointer, we're adding on rhs, hence multiply rhs by lhs's ptr size */
					e->rhs = expr_ptr_multiply(e->rhs, e->lhs->tree_type);
				else
					e->lhs = expr_ptr_multiply(e->lhs, e->rhs->tree_type);

				const_fold(e);
			}else{
				cc1_warn_at(&e->tree_type->type->where, 0, WARN_VOID_ARITH, "arithmetic with void pointer");
			}
		}

		/* check types */
		if(e->rhs)
			fold_decl_equal(e->lhs->tree_type, e->rhs->tree_type, &e->where, WARN_COMPARE_MISMATCH,
					"operation between mismatching types");
	}
}
