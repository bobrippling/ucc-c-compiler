static type_ref *type_ref_next_1(type_ref *r)
{
	if(r->type == type_ref_tdef){
		/* typedef - jump to its typeof */
		struct type_ref_tdef *tdef = &r->bits.tdef;
		decl *preferred = tdef->decl;

		r = preferred ? preferred->ref : tdef->type_of->tree_type;

		UCC_ASSERT(r, "unfolded typeof()");

		return r;
	}

	return r->ref;
}

static type_ref *type_ref_skip_tdefs_casts(type_ref *r)
{
	while(r)
		switch(r->type){
			case type_ref_tdef:
			case type_ref_cast:
				r = type_ref_next_1(r);
				continue;
			default:
				goto fin;
		}

fin:
	return r;
}

type_ref *type_ref_skip_casts(type_ref *r)
{
	while(r && r->type == type_ref_cast)
		r = type_ref_next_1(r);

	return r;
}

decl *type_ref_is_tdef(type_ref *r)
{
	if(r && r->type == type_ref_tdef)
		return r->bits.tdef.decl;

	return NULL;
}

type_ref *type_ref_next(type_ref *r)
{
	if(!r)
		return NULL;

	switch(r->type){
		case type_ref_type:
			ICE("%s on type", __func__);

		case type_ref_tdef:
		case type_ref_cast:
			return type_ref_next(type_ref_skip_tdefs_casts(r));

		case type_ref_ptr:
		case type_ref_block:
		case type_ref_func:
		case type_ref_array:
			return r->ref;
	}

	ucc_unreach(NULL);
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

type_ref *type_ref_is_ptr(type_ref *r)
{
	r = type_ref_is(r, type_ref_ptr);
	return r ? r->ref : NULL;
}

type_ref *type_ref_is_array(type_ref *r)
{
	r = type_ref_is(r, type_ref_array);
	return r ? r->ref : NULL;
}

type_ref *type_ref_is_scalar(type_ref *r)
{
	if(type_ref_is_s_or_u(r) || type_ref_is_array(r))
		return NULL;
	return r;
}

type_ref *type_ref_is_func_or_block(type_ref *r)
{
	type_ref *t = type_ref_is(r, type_ref_func);
	if(t)
		return t;

	t = type_ref_is(r, type_ref_block);
	if(t){
		t = type_ref_next(t);
		UCC_ASSERT(t->type == type_ref_func,
				"block->next not func?");
		return t;
	}

	return NULL;
}

const type *type_ref_get_type(type_ref *r)
{
	for(; r; )
		switch(r->type){
			case type_ref_tdef:
				r = type_ref_skip_tdefs_casts(r);
				break;
			case type_ref_type:
				return r->bits.type;
			default:
				goto no;
		}

no:
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

static type_ref *decl_is(decl *d, enum type_ref_type t)
{
	return type_ref_is(d->ref, t);
}

static int decl_is_ptr(decl *d)
{
	return !!decl_is(d, type_ref_ptr);
}

int type_ref_is_fptr(type_ref *r)
{
	return !!type_ref_is(type_ref_is_ptr(r), type_ref_func);
}

int type_ref_is_nonfptr(type_ref *r)
{
	if((r = type_ref_is_ptr(r)))
		return !type_ref_is(r, type_ref_func);

	return 0; /* not a ptr */
}

int type_ref_is_void_ptr(type_ref *r)
{
	return !!type_ref_is_type(type_ref_is_ptr(r), type_void);
}

int type_ref_is_nonvoid_ptr(type_ref *r)
{
	if((r = type_ref_is_ptr(r)))
		return !type_ref_is_type(r, type_void);
	return 0;
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

unsigned type_ref_align(type_ref *r, where *from)
{
	struct_union_enum_st *sue;
	type_ref *test;

	if((sue = type_ref_is_s_or_u(r)))
		/* safe - can't have an instance without a ->sue */
		return sue->align;

	if(type_ref_is(r, type_ref_ptr)
	|| type_ref_is(r, type_ref_block))
	{
		return platform_word_size();
	}

	if((test = type_ref_is(r, type_ref_type)))
		return type_align(test->bits.type, from);

	if((test = type_ref_is(r, type_ref_array)))
		return type_ref_align(test->ref, from);

	return 1;
}

int type_ref_is_complete(type_ref *r)
{
	/* decl is "void" or incomplete-struct or array[] */
	r = type_ref_skip_tdefs_casts(r);

	switch(r->type){
		case type_ref_type:
		{
			const type *t = r->bits.type;

			switch(t->primitive){
				case type_void:
					return 0;
				case type_struct:
				case type_union:
				case type_enum:
					return sue_complete(t->sue);

				default:break;
			}

			break;
		}

		case type_ref_array:
			return r->bits.array.size && type_ref_is_complete(r->ref);

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

int type_ref_is_variably_modified(type_ref *r)
{
	/* vlas not implemented yet */
#if 0
	if(type_ref_is_array(r)){
		/* ... */
	}
#else
	(void)r;
#endif
	return 0;
}

enum type_ref_str_type
type_ref_str_type(type_ref *r)
{
	type_ref *t = type_ref_is_array(r);
	if(!t)
		t = type_ref_is_ptr(r);
	t = type_ref_is_type(t, type_unknown);
	switch(t ? t->bits.type->primitive : type_unknown){
		case type_char: return type_ref_str_char;
		case type_int: return type_ref_str_wchar;
		default: return type_ref_str_no;
	}
}

int type_ref_is_incomplete_array(type_ref *r)
{
	if((r = type_ref_is(r, type_ref_array)))
		return !r->bits.array.size;

	return 0;
}

type_ref *type_ref_complete_array(type_ref *r, int sz)
{
	r = type_ref_is(r, type_ref_array);

	UCC_ASSERT(r, "not an array");

	EOF_WHERE(&r->where,
		r = type_ref_new_array(r->ref, expr_new_val(sz))
	);
	return r;
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
	fp = type_ref_skip_tdefs_casts(fp);
	switch(fp->type){
		case type_ref_ptr:
		case type_ref_block:
			fp = type_ref_is(fp->ref, type_ref_func);
			UCC_ASSERT(fp, "not a func for fcall");
			/* fall */

		case type_ref_func:
			if(pfuncargs)
				*pfuncargs = fp->bits.func.args;
			fp = fp->ref;
			UCC_ASSERT(fp, "no ref for func");
			break;

		default:
			ICE("can't func-deref non func-ptr/block ref (%d)", fp->type);
	}

	return fp;
}

type_ref *type_ref_decay(type_ref *r)
{
	/* f(int x[][5]) decays to f(int (*x)[5]), not f(int **x) */

	r = type_ref_skip_tdefs_casts(r);

	switch(r->type){
		case type_ref_array:
		{
			/* don't mutate a type_ref */
			type_ref *new = type_ref_new_ptr(r->ref, qual_none);
			new->bits.ptr = r->bits.array; /* save the old size, etc */
			return new;
		}

		case type_ref_func:
			return type_ref_new_ptr(r, qual_none);

		default:
			break;
	}

	return r;
}

int type_ref_is_void(type_ref *r)
{
	return !!type_ref_is_type(r, type_void);
}

int type_ref_is_signed(type_ref *r)
{
	/* need to take casts into account */
	while(r)
		switch(r->type){
			case type_ref_type:
				return r->bits.type->is_signed;

			case type_ref_cast:
				if(r->bits.cast.is_signed_cast)
					return r->bits.cast.signed_true;
				/* fall */

			default:
				r = type_ref_next_1(r);
		}

	return 0;
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
		case type_ref_type:
			if(r->bits.type->primitive == type_struct
			|| r->bits.type->primitive == type_union)
			{
				if(r->bits.type->sue->contains_const)
					return qual_const;
			}

		case type_ref_func:
		case type_ref_array:
			return qual_none;

		case type_ref_cast:
			/* descend */
			if(r->bits.cast.is_signed_cast)
				return type_ref_qual(r->ref);
			return r->bits.cast.qual
				| (r->bits.cast.additive ? type_ref_qual(r->ref) : qual_none);

		case type_ref_ptr:
		case type_ref_block:
			return r->bits.ptr.qual; /* no descend */

		case type_ref_tdef:
			return type_ref_qual(r->bits.tdef.type_of->tree_type);
	}

	ucc_unreach(qual_none);
}

funcargs *type_ref_funcargs(type_ref *r)
{
	type_ref *test;

	if((test = type_ref_is(r, type_ref_ptr))
	|| (test = type_ref_is(r, type_ref_block)))
	{
		r = test->ref; /* jump down past the (*)() */
	}

	UCC_ASSERT(r, "not a function type");

	return r->bits.func.args;
}

int type_ref_is_callable(type_ref *r)
{
	type_ref *test;

	r = type_ref_skip_tdefs_casts(r);

	if((test = type_ref_is(r, type_ref_ptr)) || (test = type_ref_is(r, type_ref_block)))
		return !!type_ref_is(test->ref, type_ref_func);

	return 0;
}

int type_ref_is_const(type_ref *r)
{
	/* const char *x is not const. char *const x is */
	return !!(type_ref_qual(r) & qual_const);
}

long type_ref_array_len(type_ref *r)
{
	r = type_ref_is(r, type_ref_array);

	UCC_ASSERT(r, "not an array");
	UCC_ASSERT(r->bits.array.size, "array len of []");

	return const_fold_val(r->bits.array.size);
}
