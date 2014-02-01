#include <stdio.h>
#include <assert.h>

#include "../util/alloc.h"
#include "cc1_where.h"

#include "btype.h"
#include "type.h"

#include "type_nav.h"
#include "type_is.h"

#include "const.h"
#include "funcargs.h"
#include "c_types.h"

struct type_nav
{
	type **btypes; /* indexed by type_primitive */
	dynmap *suetypes; /* sue => type */
	type *tva_list;
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

	if(!candidate->bits.array.size)
		return 0;

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

struct ctx_func
{
	funcargs *args;
	symtable *arg_scope;
};

static void init_func(type *ty, void *ctx)
{
	struct ctx_func *c = ctx;
	ty->bits.func.args = c->args;
	ty->bits.func.arg_scope = c->arg_scope;
}

static int eq_func(type *ty, void *ctx)
{
	struct ctx_func *c = ctx;

	if(c->arg_scope != ty->bits.func.arg_scope)
		return 0;

	if(funcargs_cmp(ty->bits.func.args, c->args) == FUNCARGS_EXACT_EQUAL){
		funcargs_free(ctx, 0);
		return 1;
	}
	return 0;
}

type *type_func_of(type *ty_ret,
		struct funcargs *args, struct symtable *arg_scope)
{
	struct ctx_func ctx;
	ctx.args = args;
	ctx.arg_scope = arg_scope;

	return type_uptree_find_or_new(
			ty_ret, type_func,
			eq_func, init_func,
			&ctx);
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
	if(!attr)
		return ty;

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
	if(!qual)
		return unqualified;

	return type_uptree_find_or_new(
			unqualified, type_cast,
			eq_qual, init_qual,
			&qual);
}

static int eq_sign(type *candidate, void *ctx)
{
	if(!candidate->bits.cast.is_signed_cast)
		return 0;
	return candidate->bits.cast.signed_true == *(int *)ctx;
}

static void init_sign(type *t, void *ctx)
{
	t->bits.cast.is_signed_cast = 1;
	t->bits.cast.signed_true = *(int *)ctx;
}

type *type_sign(type *ty, int is_signed)
{
	return type_uptree_find_or_new(
			ty, type_cast,
			eq_sign, init_sign,
			&is_signed);
}

struct ctx_tdef
{
	expr *e;
	decl *d;
};

static int eq_tdef(type *candidate, void *ctx)
{
	struct ctx_tdef *c = ctx;
	return candidate->bits.tdef.type_of == c->e
		&&   candidate->bits.tdef.decl == c->d;
}

static void init_tdef(type *candidate, void *ctx)
{
	struct ctx_tdef *c = ctx;
	candidate->bits.tdef.type_of = c->e;
	candidate->bits.tdef.decl = c->d;
}

type *type_tdef_of(expr *e, decl *d)
{
	struct ctx_tdef ctx;

	assert(e);
	ctx.e = e;
	ctx.d = d;

#if 0
	/* TODO: this won't work until parse() and fold() are merged */
	return type_uptree_find_or_new(
			e->tree_type, type_tdef,
			eq_tdef, init_tdef,
			&ctx);
#else
	{
		type *t = type_new(type_tdef, NULL);
		init_tdef(t, &ctx);
		return t;
	}
#endif
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

type *type_nav_MAX_FOR(struct type_nav *root, unsigned sz)
{
	enum type_primitive prims[] = {
		type_llong, type_long, type_int, type_short, type_nchar
	};
	unsigned i;

	for(i = 0; i < sizeof(prims)/sizeof(*prims); i++)
		if(sz >= type_primitive_size(prims[i]))
			return type_nav_btype(root, prims[i]);

	return NULL;
}

type *type_unqualify(type *t)
{
	while(t)
		if(t->type == type_cast && !t->bits.cast.is_signed_cast)
			t = t->ref;
		else
			break;

	return t;
}

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

type *type_nav_suetype(struct type_nav *root, struct_union_enum_st *sue)
{
	type *ent;
	btype *bt;

	if(!root->suetypes)
		root->suetypes = dynmap_new(/*refeq:*/NULL);

	ent = dynmap_get(struct_union_enum_st *, type *, root->suetypes, sue);

	if(ent)
		return ent;

	bt = umalloc(sizeof *bt);
	bt->primitive = sue->primitive;
	bt->sue = sue;
	ent = type_new_btype(bt);

	dynmap_set(struct_union_enum_st *, type *, root->suetypes, sue, ent);

	return ent;
}

type *type_nav_va_list(struct type_nav *root)
{
	if(!root->tva_list)
		root->tva_list = c_types_make_va_list();

	return root->tva_list;
}

static void type_dump_t(type *t, FILE *f, int indent)
{
	int i;
	for(i = 0; i < indent; i++)
		putchar(' ');

	fprintf(f, "%s %s\n",
			type_kind_to_str(t->type),
			type_to_str(t));

	if(t->uptree){
		indent++;

		for(i = 0; i < N_TYPE_KINDS; i++){
			struct type_tree_ent *ent;
			for(ent = t->uptree->ups[i]; ent; ent = ent->next)
				if(ent->t)
					type_dump_t(ent->t, f, indent);
		}

		indent--;
	}
}

void type_nav_dump(struct type_nav *nav)
{
	int i;
	for(i = 0; i < N_TYPE_KINDS; i++){
		type *t = nav->btypes[i];
		if(t)
			type_dump_t(t, stderr, 0);
	}
}
