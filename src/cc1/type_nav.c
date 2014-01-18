#include <assert.h>

#include "../util/alloc.h"
#include "cc1_where.h"

#include "btype.h"
#include "type.h"

#include "type_nav.h"
#include "type_is.h"

#include "const.h"
#include "funcargs.h"

struct type_nav
{
	type **btypes; /* indexed by type_primitive */
};

struct type_tree
{
	struct type_tree_ent
	{
		type *t;
		struct type_tree_ent *next;
	} *ups[N_TYPE_KINDS];
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

static type *type_uptree_find_or_new(
		type *to, enum type_kind idx,
		int (*eq)(type *, void *),
		void (*init)(type *, void *),
		void *ctx)
{
	struct type_tree_ent **ent;

	if(!to->uptree)
		to->uptree = umalloc(sizeof *to->uptree);

	for(ent = &to->uptree->ups[idx]; *ent; ent = &(*ent)->next){
		type *candidate = (*ent)->t;
		assert(candidate->type == idx);

		if(!eq || eq(candidate, ctx))
			return candidate;
	}

	{
		type *new_t = type_new(idx, to);

		if(init)
			init(new_t, ctx);

		*ent = umalloc(sizeof **ent);
		(*ent)->t = new_t;
		return new_t;
	}
}

struct ctx_array
{
	expr *sz;
	integral_t sz_i;
	int is_static;
};

static int eq_array(type *candidate, void *ctx)
{
	struct ctx_array *c = ctx;

	if(candidate->bits.array.is_static != c->is_static)
		return 0;

	if(candidate->bits.array.size == c->sz){
		/* including [] */
		return 1;
	}

	return c->sz_i == const_fold_val_i(candidate->bits.array.size);
}

static void init_array(type *ty, void *ctx)
{
	struct ctx_array *c = ctx;
	ty->bits.array.size = c->sz;
	ty->bits.array.is_static = c->is_static;
}

type *type_array_of_static(type *to, struct expr *new_sz, int is_static)
{
	struct ctx_array ctx;
	ctx.sz_i = new_sz ? const_fold_val_i(new_sz) : 0;
	ctx.sz = new_sz;
	ctx.is_static = is_static;

	return type_uptree_find_or_new(
			to, type_array,
			eq_array, init_array,
			&ctx);
}

type *type_array_of(type *to, struct expr *new_sz)
{
	return type_array_of_static(to, new_sz, 0);
}

static void init_func(type *ty, void *ctx)
{
	ty->bits.func.args = ctx;
}

static int eq_func(type *ty, void *ctx)
{
	if(funcargs_cmp(ty->bits.func.args, ctx) == FUNCARGS_ARE_EQUAL){
		funcargs_free(ctx, 0);
		return 1;
	}
	return 0;
}

type *type_func_of(type *ty_ret, struct funcargs *args)
{
	return type_uptree_find_or_new(
			ty_ret, type_func,
			eq_func, init_func,
			args);
}

type *type_block_of(type *fn)
{
	return type_uptree_find_or_new(
			fn, type_block,
			NULL, NULL, NULL);
}

static int eq_attr(type *candidate, void *ctx)
{
	if(attribute_equal(candidate->bits.attr, ctx)){
		attribute_free(ctx);
		return 1;
	}
	return 0;
}

static void init_attr(type *ty, void *ctx)
{
	ty->bits.attr = ctx;
}

type *type_attributed(type *ty, attribute *attr)
{
	return type_uptree_find_or_new(
			ty, type_attr,
			eq_attr, init_attr,
			attr);
}

type *type_ptr_to(type *pointee)
{
	return type_uptree_find_or_new(
			pointee, type_ptr,
			NULL, NULL, NULL);
}

static int eq_qual(type *candidate, void *ctx)
{
	if(candidate->bits.cast.is_signed_cast)
		return 0;
	return candidate->bits.cast.qual == *(enum type_qualifier *)ctx;
}

static void init_qual(type *t, void *ctx)
{
	t->bits.cast.qual = *(enum type_qualifier *)ctx;
}

type *type_qualify(type *unqualified, enum type_qualifier qual)
{
	return type_uptree_find_or_new(
			unqualified, type_cast,
			eq_qual, init_qual,
			&qual);
}

type *type_called(type *functy, struct funcargs **pfuncargs)
{
	functy = type_skip_tdefs_casts(functy);
	assert(functy->type == type_func);
	if(pfuncargs)
		*pfuncargs = functy->bits.func.args;
	return functy->ref;
}

type *type_pointed_to(type *ty)
{
	type *const r_save = ty;

	ty = type_is_ptr(ty);
	assert(ty);

	/* *(void (*)()) does nothing */
	if(type_is(ty, type_func))
		return r_save;

	return ty;
}

#if 0
TODO:

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
