#include <assert.h>

#include "../util/alloc.h"
#include "cc1_where.h"

#include "btype.h"
#include "type.h"

#include "type_nav.h"

struct type_nav
{
	type **btypes; /* indexed by type_primitive */
};

struct type_nav *cc1_type_nav;

struct type_nav *type_nav_init(void)
{
	struct type_nav *root = umalloc(sizeof *root);
	root->btypes = ucalloc(type_unknown, sizeof *root->btypes);
	return root;
}

static type *type_new(enum type_kind t, type *of)
{
	type *r = umalloc(sizeof *r);
	if(of)
		memcpy_safe(&r->where, &of->where);
	else
		where_cc1_current(&r->where);

	r->type = t;
	r->ref = of;
	return r;
}

static type *type_new_btype(const btype *b)
{
	type *t = type_new(type_btype, NULL);
	t->bits.type = b;
	return t;
}

#if 0
TODO:

type_array_of
type_array_of_qual
type_block_of
type_called
type_func_of
type_pointed_to
type_ptr_to
type_qualify
type_nav_MAX_FOR
type_nav_suetype
type_nav_va_list
type_sign
type_tdef_of
type_unqualify

and obviously the type navigation code

#endif

#if 0
static type *type_new_type_qual(enum type_primitive t, enum type_qualifier q)
{
	return type_new_cast(
			type_new_btype(type_new_primitive(t)),
			q);
}

static type *type_new_tdef(struct expr *e, struct decl *to)
{
	type *r = type_new(type_tdef, NULL);
	UCC_ASSERT(expr_kind(e, sizeof), "not sizeof for tdef ref");
	r->bits.tdef.type_of = e;
	r->bits.tdef.decl = to; /* NULL if typeof */
	return r;
}

static type *type_new_ptr(type *to, enum type_qualifier q)
{
	type *r = type_new(type_ptr, to);
	r->bits.ptr.qual = q;
	return r;
}

static type *type_new_block(type *to, enum type_qualifier q)
{
	type *r = type_new_ptr(to, q);
	r->type = type_block;
	return r;
}

static type *type_new_array(type *to, struct expr *sz)
{
	type *r = type_new(type_array, to);
	r->bits.array.size = sz;
	return r;
}

static type *type_new_array2(type *to, struct expr *sz,
		enum type_qualifier q, int is_static)
{
	type *r = type_new_array(to, sz);
	r->bits.array.is_static = is_static;
	r->bits.array.qual      = q;
	return r;
}

static type *type_new_func(
		type *of, struct funcargs *args, struct symtable *arg_scope)
{
	type *r = type_new(type_func, of);
	r->bits.func.args = args;
	r->bits.func.arg_scope = arg_scope;
	return r;
}

static type *type_cached_MAX_FOR(unsigned sz)
{
	enum type_primitive prims[] = {
		type_long, type_int, type_short, type_nchar
	};
	unsigned i;

	for(i = 0; i < sizeof(prims)/sizeof(*prims); i++)
		if(sz >= type_primitive_size(prims[i]))
			return type_new_btype(type_new_primitive(prims[i]));
	return NULL;
}

static type *type_new_cast_is_additive(
		type *to, enum type_qualifier new, int additive)
{
	type *r;

	if(!new)
		return to;

	r = type_new(type_cast, to);
	r->bits.cast.qual = new;
	r->bits.cast.additive = additive;
	return r;
}

static type *type_new_cast(type *to, enum type_qualifier new)
{
	return type_new_cast_is_additive(to, new, 0);
}

static type *type_new_cast_add(type *to, enum type_qualifier add)
{
	return type_new_cast_is_additive(to, add, 1);
}

static type *type_new_cast_signed(type *to, int is_signed)
{
	type *r = type_new(type_cast, to);

	r->bits.cast.is_signed_cast = 1;
	r->bits.cast.signed_true = is_signed;

	return r;
}
#endif

type *type_nav_btype(struct type_nav *root, enum type_primitive p)
{
	assert(p < type_struct);

	if(!root->btypes[p]){
		btype *b = umalloc(sizeof *b);
		b->primitive = p;
		root->btypes[p] = type_new_btype(b);
	}

	return root->btypes[p];
}

#if 0
type *type_ptr_depth_dec(type *r, where *w)
{
	type *const r_save = r;

	r = type_is_ptr(r);

	if(!r){
		die_at(w,
				"invalid indirection applied to %s",
				r_save ? type_to_str(r_save) : "(NULL)");
	}

	/* *(void (*)()) does nothing */
	if(type_is(r, type_func))
		return r_save;

	/* don't check for incomplete types here */

	/* XXX: memleak */
	/*type_free(r_save);*/

	return r;
}

type *type_ptr_depth_inc(type *r)
{
	type *test;
	if((test = type_is_primitive(r, type_unknown))){
		type *p = cache_ptr[test->bits.type->primitive];
		if(p)
			return p;
	}

	return type_new_ptr(r, qual_none);
}
#endif
