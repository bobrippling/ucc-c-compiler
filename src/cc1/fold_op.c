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

	if(e->rhs->type != expr_identifier)
		die_at(&e->rhs->where, "struct member must be an identifier");
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

	GET_TREE_TYPE_TO(e->rhs, d);

	/*
	 * if it's a.b, convert to (&a)->b for asm gen
	 * e = { lhs = "a", rhs = "b", type = dot }
	 * e = { lhs = { expr = "a", type = addr }, rhs = "b", type = ptr }
	 */
	if(ptr_depth_exp == 0){
		expr *new = expr_new();

		new->expr = e->lhs;
		e->lhs = new;
		new->type = expr_addr;

		e->type = expr_op;
		e->op   = op_struct_ptr;

		fold_expr(e->lhs, stab);
		GET_TREE_TYPE(e->lhs->tree_type);
	}else{
		GET_TREE_TYPE(d);
	}
}

void fold_op_typecheck(expr *e, symtable *stab)
{
	enum {
		SIGNED, UNSIGNED
	} rhs, lhs;
	type *type_l, *type_r;

	(void)stab;

	if(!e->rhs)
		return;

	type_l = e->lhs->tree_type->type;
	type_r = e->rhs->tree_type->type;

	if(type_l->primitive == type_enum
			&& type_r->primitive == type_enum
			&& type_l->enu != type_r->enu){
		cc1_warn_at(&e->where, 0, WARN_ENUM_CMP, "comparison between enum %s and enum %s", type_l->spel, type_r->spel);
	}


	lhs = type_l->spec & spec_unsigned ? UNSIGNED : SIGNED;
	rhs = type_r->spec & spec_unsigned ? UNSIGNED : SIGNED;

	if(op_is_cmp(e->op) && rhs != lhs){
#define SIGN_CONVERT(test_hs, assert_hs) \
		if(e->test_hs->type == expr_val && e->test_hs->val.i.val >= 0){ \
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

#define SPEL_IF_IDENT(hs)                              \
				hs->type == expr_identifier ? " ("     : "", \
				hs->type == expr_identifier ? hs->spel : "", \
				hs->type == expr_identifier ? ")"      : ""  \

		cc1_warn_at(&e->where, 0, WARN_SIGN_COMPARE, "comparison between signed and unsigned%s%s%s%s%s%s",
				SPEL_IF_IDENT(e->lhs), SPEL_IF_IDENT(e->rhs));
	}
noproblem:
	return;
}

void fold_op(expr *e, symtable *stab)
{
	if(e->op == op_struct_ptr || e->op == op_struct_dot){
		fold_op_struct(e, stab);
		return;
	}

	fold_expr(e->lhs, stab);
	if(e->rhs)
		fold_expr(e->rhs, stab);

	fold_op_typecheck(e, stab);

	/* XXX: note, this assumes that e.g. "1 + 2" the lhs and rhs have the same type */
	if(e->op == op_deref){
		/* check for *&x */

		if(e->lhs->type == expr_addr)
			warn_at(&e->lhs->where, "possible optimisation for *& expression");


		GET_TREE_TYPE(e->lhs->tree_type);

		e->tree_type = decl_ptr_depth_dec(e->tree_type);

		if(decl_ptr_depth(e->tree_type) == 0)
			switch(e->lhs->tree_type->type->primitive){
				case type_unknown:
				case type_void:
					die_at(&e->where, "can't dereference void pointer");
				default:
					/* e->tree_type already set to deref type */
					break;
			}
	}else{
		/*
		 * look either side - if either is a pointer, take that as the tree_type
		 *
		 * operation between two values of any type
		 *
		 * TODO: checks for pointer + pointer (invalid), etc etc
		 */

#define IS_PTR(x) decl_ptr_depth(x->tree_type)

		if(e->rhs){
			if(e->op == op_minus && IS_PTR(e->lhs) && IS_PTR(e->rhs))
				e->tree_type->type->primitive = type_int;
			else if(IS_PTR(e->rhs))
				GET_TREE_TYPE(e->rhs->tree_type);
			else
				goto norm_tt;
		}else{
norm_tt:
			GET_TREE_TYPE(e->lhs->tree_type);
		}
	}

	if(e->rhs){
		/* need to do this check _after_ we get the correct tree type */
		if((e->op == op_plus || e->op == op_minus) &&
				decl_ptr_depth(e->tree_type) &&
				e->rhs){

			/* 2 + (void *)5 is 7, not 2 + 8*5 */
			if(e->tree_type->type->primitive != type_void){
				/* we're dealing with pointers, adjust the amount we add by */

				if(decl_ptr_depth(e->lhs->tree_type))
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
