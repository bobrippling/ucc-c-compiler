static type_ref *type_ref_skip_tdefs_casts(type_ref *r)
{
	while(r){
		switch(r->type){
			case type_ref_tdef:
			{
				/* typedef - jump to its typeof */
				struct type_ref_tdef *tdef = &r->bits.tdef;
				decl *preferred = tdef->decl;

				r = preferred ? preferred->ref : tdef->type_of->tree_type;

				continue;
			}

			case type_ref_cast:
				break;

			default:
				goto fin;
		}
		r = r->ref;
	}

fin:
	return r;
}

type_ref *type_ref_is(type_ref *r, enum type_ref_type t)
{
	r = type_ref_skip_tdefs_casts(r);

	if(!r || r->type != t)
		return NULL;

	return r;
}

type_ref *type_ref_is_type(type_ref *r, enum type_primitive p)
{
	r = type_ref_is(r, type_ref_type);

	/* extra checks for a type */
	if(r && (p == type_unknown || r->bits.type->primitive == p))
		return r;

	return NULL;
}

int type_ref_is_bool(type_ref *r)
{
	if(type_ref_is(r, type_ref_ptr))
		return 1;

	r = type_ref_is(r, type_ref_type);

	if(!r)
		return 0;

	switch(r->bits.type->primitive){
		case type__Bool:
		case type_char:
		case type_int:
		case type_short:
		case type_long:
		case type_llong:
			return 1;
		default:
			return 0;
	}
}

type_ref *decl_is(decl *d, enum type_ref_type t)
{
	return type_ref_is(d->ref, t);
}

int decl_is_ptr(decl *d)
{
	return !!decl_is(d, type_ref_ptr);
}

int decl_is_block(decl *d)
{
	return !!decl_is(d, type_ref_block);
}

int decl_is_func(decl *d)
{
	return !!decl_is(d, type_ref_func);
}

enum type_primitive type_ref_type_primitive(decl *d)
{
	const type_ref *r = type_ref_skip_tdefs_casts(d->ref);
	return r->type == type_ref_type ? r->bits.type->primitive : type_unknown;
}

int decl_is_struct_or_union(decl *d)
{
	enum type_primitive t = type_ref_type_primitive(d);
	return t == type_struct || t == type_union;
}

int type_ref_is_fptr(type_ref *r)
{
	return (r = type_ref_is(r, type_ref_ptr))
		&& type_ref_is(r, type_ref_func);
}

int type_ref_is_void_ptr(type_ref *r)
{
	if((r = type_ref_is(r, type_ref_ptr)))
		return !!type_ref_is_type(r->ref, type_void);

	return 0;
}

int decl_is_integral(decl *d)
{
	return type_ref_is_integral(d->ref);
}

int decl_complete(decl *d)
{
	return type_ref_is_complete(d->ref);
}

int type_ref_is_integral(type_ref *r)
{
	r = type_ref_is(r, type_ref_type);

	if(!r)
		return 0;

	switch(r->bits.type->primitive){
		case type_int:
		case type_char:
		case type__Bool:
		case type_short:
		case type_long:
		case type_llong:
		case type_enum:
		case type_intptr_t:
		case type_ptrdiff_t:
			return 1;

		case type_unknown:
		case type_void:
		case type_struct:
		case type_union:
		case type_float:
		case type_double:
		case type_ldouble:
			break;
	}

	return 0;
}

int type_ref_align(type_ref *r, where const *from)
{
	struct_union_enum_st *sue;

	if((sue = type_ref_is_s_or_u(r)))
		/* safe - can't have an instance without a ->sue */
		return sue->align;

	if((r = type_ref_is(r, type_ref_ptr))
	|| (r = type_ref_is(r, type_ref_block)))
	{
		return type_primitive_size(type_intptr_t);
	}

	if((r = type_ref_is(r, type_ref_type)))
		return type_size(r->bits.type, from);

	return 1;
}

int type_ref_is_complete(type_ref *r)
{
	/* decl is "void" or incomplete-struct or array[] */
	r = type_ref_skip_tdefs_casts(r);

	switch(r->type){
		case type_ref_type:
		{
			type *t = r->bits.type;

			switch(t->primitive){
				case type_void:
					return 0;
				case type_struct:
				case type_union:
				case type_enum:
					return !sue_incomplete(t->sue);

				default:break;
			}

			break;
		}

		case type_ref_array:
		{
			intval iv;

			const_fold_need_val(r->bits.array_size, &iv);

			return iv.val != 0 && type_ref_is_complete(r->ref);
		}

		case type_ref_func:
		case type_ref_ptr:
		case type_ref_block:
			break;

		case type_ref_tdef:
		case type_ref_cast:
			ICE("should've been skipped");
	}


	return 1;
}

struct_union_enum_st *type_ref_is_s_or_u_or_e(type_ref *r)
{
	type_ref *test = type_ref_is(r, type_ref_type);

	if(!test)
		return NULL;

	return test->bits.type->sue; /* NULL if not s/u/e */
}

struct_union_enum_st *type_ref_is_s_or_u(type_ref *r)
{
	struct_union_enum_st *sue = type_ref_is_s_or_u_or_e(r);
	if(sue && sue->primitive != type_enum)
		return sue;
	return NULL;
}

type_ref *type_ref_func_call(type_ref *fp, funcargs **pfuncargs)
{
	switch(fp->type){
		case type_ref_ptr:
		case type_ref_block:
			fp = type_ref_is(fp->ref, type_ref_func);
			UCC_ASSERT(fp, "not a func for fcall");
			/* fall */

		case type_ref_func:
			if(pfuncargs)
				*pfuncargs = fp->bits.func;
			fp = fp->ref;
			break;

		default:
			ICE("can't func-deref non func-ptr/block ref");
	}

	return fp;
}

type_ref *type_ref_decay(type_ref *r)
{
	/* f(int x[][5]) decays to f(int (*x)[5]), not f(int **x) */

	switch(r->type){
		default:break;

		case type_ref_array:
			r = r->ref;
			/* XXX: memleak */
			/* fall */

		case type_ref_func:
			return type_ref_new_ptr(r, qual_none);
	}

	return r;
}

int type_ref_is_void(type_ref *r)
{
	return !!type_ref_is_type(r, type_void);
}

int type_ref_is_signed(type_ref *r)
{
	r = type_ref_is(r, type_ref_type);

	return r && r->bits.type->is_signed;
}

int type_ref_is_floating(type_ref *r)
{
	r = type_ref_is(r, type_ref_type);

	if(!r)
		return 0;

	switch(r->bits.type->primitive){
		case type_float:
		case type_double:
		case type_ldouble:
			return 1;
		default:
			break;
	}
	return 0;
}

enum type_qualifier type_ref_qual(const type_ref *r)
{
	/* stop at the first pointer or type, collecting from type_ref_cast quals */

	if(!r)
		return qual_none;

	switch(r->type){
		case type_ref_func:
		case type_ref_array:
			return qual_none;

		case type_ref_cast:
			/* descend */
			return r->bits.cast.qual | (r->bits.cast.additive ? type_ref_qual(r->ref) : qual_none);

		case type_ref_type:
			return r->bits.type->qual;

		case type_ref_ptr:
		case type_ref_block:
			return r->bits.qual; /* no descend */

		case type_ref_tdef:
			return type_ref_qual(r->bits.tdef.type_of->tree_type);
	}

	ucc_unreach();
}

funcargs *type_ref_funcargs(type_ref *r)
{
	type_ref *test;

	if((test = type_ref_is(r, type_ref_ptr)))
		r = test->ref; /* jump down past the (*)() */

	r = type_ref_is(r, type_ref_func);

	return r ? r->bits.func : NULL;
}

int type_ref_is_callable(type_ref *r)
{
	type_ref *test;

	if((test = type_ref_is(r, type_ref_ptr)) || (test = type_ref_is(r, type_ref_block)))
		return !!type_ref_is(test->ref, type_ref_func);

	return 0;
}

int type_ref_is_const(type_ref *r)
{
	/* const char *x is not const. char *const x is */
	return !!(type_ref_qual(r) & qual_const);
}

int type_ref_array_len(type_ref *r)
{
	intval iv;

	r = type_ref_is(r, type_ref_array);

	UCC_ASSERT(r, "not an array");

	const_fold_need_val(r->bits.array_size, &iv);

	return iv.val;
}
