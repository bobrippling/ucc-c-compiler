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
	dynmap *enumints; /* int types covering an enum, sue => type */
	dynmap *suetypes; /* sue => type */
	type *tva_list, *tauto;
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

static int eq_true(type *t, void *ctx)
{
	(void)t; (void)ctx;
	return 1;
}

static int eq_false(type *t, void *ctx)
{
	(void)t; (void)ctx;
	return 0;
}

static ucc_nonnull((1, 3))
type *type_uptree_find_or_new(
		type *to, enum type_kind idx,
		int (*eq)(type *, void *),
		void (*init)(type *, void *),
		void *ctx)
{
	struct type_tree_ent **ent;

	to = type_skip_wheres(to);

	if(!to->uptree)
		to->uptree = umalloc(sizeof *to->uptree);

	for(ent = &to->uptree->ups[idx]; *ent; ent = &(*ent)->next){
		type *candidate = (*ent)->t;
		assert(candidate->type == idx);

		if(eq(candidate, ctx))
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
	int is_vla;
};

static int eq_array(type *candidate, void *ctx)
{
	struct ctx_array *c = ctx;
	consty k;

	assert(candidate->type == type_array);

	if(candidate->bits.array.is_static != c->is_static)
		return 0;

	if(candidate->bits.array.is_vla != c->is_vla)
		return 0;

	if(candidate->bits.array.size == c->sz){
		/* including [] */
		return 1;
	}

	if(!candidate->bits.array.size || !c->sz)
		return 0;

	const_fold(candidate->bits.array.size, &k);
	if(k.type == CONST_NUM){
		assert(K_INTEGRAL(k.bits.num));
		return c->sz_i == k.bits.num.val.i;
	}else{
		/* vla - just check expression equivalence */
		return candidate->bits.array.size == c->sz;
	}
}

static void init_array(type *ty, void *ctx)
{
	struct ctx_array *c = ctx;
	ty->bits.array.size = c->sz;
	ty->bits.array.is_static = c->is_static;
	ty->bits.array.is_vla = c->is_vla;
}

static void ctx_array_init(
		struct ctx_array *ctx,
		expr *sz,
		int is_static, int is_vla)
{
	ctx->sz_i = 0;
	ctx->sz = sz;
	ctx->is_static = is_static;
	ctx->is_vla = is_vla;

	if(sz){
		consty k;
		const_fold(sz, &k);
		if(K_INTEGRAL(k.bits.num))
			ctx->sz_i = k.bits.num.val.i;
	}
}

type *type_array_of_static(type *to, struct expr *new_sz, int is_static)
{
	struct ctx_array ctx;

	ctx_array_init(&ctx, new_sz, is_static, 0);

	return type_uptree_find_or_new(
			to, type_array,
			eq_array, init_array,
			&ctx);
}

type *type_array_of(type *to, struct expr *new_sz)
{
	return type_array_of_static(to, new_sz, 0);
}

type *type_vla_of(type *of, struct expr *vlasz, int vlatype)
{
	type *vla;
	struct ctx_array ctx;

	ctx_array_init(&ctx, vlasz, 0, vlatype);

	vla = type_uptree_find_or_new(
			of, type_array,
			/* vla - not equal to any other type */
			eq_false, init_array,
			&ctx);

	return vla;
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
		funcargs_free(c->args, 0);
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
			/*if there's an uptree, we'll take it:*/eq_true,
			/*(since this is block pointer, not the block function type)*/
			NULL, NULL);
}

static int eq_attr(type *candidate, void *ctx)
{
	return attribute_equal(candidate->bits.attr, ctx);
}

static void init_attr(type *ty, void *ctx)
{
	ty->bits.attr = RETAIN((attribute *)ctx);
}

type *type_attributed(type *ty, attribute *attr)
{
	type *attributed;

	if(!attr)
		return ty;

	attributed = type_uptree_find_or_new(
			ty, type_attr,
			eq_attr, init_attr,
			attr);

	RELEASE(attr);

	return attributed;
}

struct ctx_ptr
{
	type *decayed_from;
};

static int eq_ptr(type *candidate, void *vctx)
{
	struct ctx_ptr *ctx = vctx;

	return candidate->bits.ptr.decayed_from == ctx->decayed_from;
}

static void init_ptr(type *ty, void *vctx)
{
	struct ctx_ptr *ctx = vctx;

	ty->bits.ptr.decayed_from = ctx->decayed_from;
}

type *type_ptr_to(type *pointee)
{
	struct ctx_ptr ctx = { NULL };

	return type_uptree_find_or_new(
			pointee, type_ptr,
			eq_ptr, init_ptr, &ctx);
}

struct ctx_decayed_array
{
	struct ctx_array array;
	type *decayed_from;
};

static int eq_decayed_array(type *candidate, void *ctx)
{
	struct ctx_decayed_array *actx = ctx;

	if(!candidate->bits.ptr.decayed_from)
		return 0;

	return eq_array(candidate->bits.ptr.decayed_from, &actx->array);
}

static void init_decayed_array(type *ty, void *ctx)
{
	struct ctx_decayed_array *actx = ctx;
	/* don't init array - ty is a pointer */
	assert(ty->type == type_ptr);
	ty->bits.ptr.decayed_from = actx->decayed_from;
}

type *type_decayed_ptr_to(type *pointee, type *array_from)
{
	struct ctx_decayed_array ctx;

	ctx_array_init(
			&ctx.array,
			array_from->bits.array.size,
			array_from->bits.array.is_static,
			array_from->bits.array.is_vla);

	ctx.decayed_from = array_from;

	return type_uptree_find_or_new(
			pointee, type_ptr,
			eq_decayed_array, init_decayed_array,
			&ctx);
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
	type *ar_ty;

	if(!qual)
		return unqualified;

	if((ar_ty = type_is(unqualified, type_array))){
		/* const -> array -> int
		 * becomes
		 * array -> const -> int
		 *
		 * C11 6.7.3.9 */

		return type_array_of(
				type_qualify(ar_ty->ref, qual),
				ar_ty->bits.array.size);
	}

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
	return candidate->bits.tdef.decl == c->d;
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

	return type_uptree_find_or_new(
			e->tree_type, type_tdef,
			eq_tdef, init_tdef,
			&ctx);
}

type *type_called(type *functy, struct funcargs **pfuncargs)
{
	functy = type_skip_all(functy);
	assert(functy->type == type_func);
	if(pfuncargs)
		*pfuncargs = functy->bits.func.args;
	return functy->ref;
}

type *type_dereference_decay(type *const ty_ptr)
{
	type *const pointee = type_is_ptr(ty_ptr);
	assert(pointee);

	/* *(void (*)()) does nothing */
	if(type_is(pointee, type_func))
		return ty_ptr;

	return pointee;
}

type *type_nav_MAX_FOR(struct type_nav *root, unsigned sz)
{
	enum type_primitive p = type_primitive_not_less_than_size(sz);
	if(p != type_unknown)
		return type_nav_btype(root, p);
	assert(0 && "no type max");
}

type *type_nav_int_enum(struct type_nav *root, struct_union_enum_st *enu)
{
	type *ent, *prev;
	btype *bt;

	assert(enu->primitive == type_enum && "enum?");

	if(!root->enumints){
		root->enumints = dynmap_new(
				struct_union_enum_st *, /*refeq:*/NULL, sue_hash);
	}

	ent = dynmap_get(struct_union_enum_st *, type *, root->enumints, enu);

	if(ent)
		return ent;

	bt = umalloc(sizeof *bt);
	bt->primitive = type_int;
	bt->sue = enu;
	ent = type_new_btype(bt);

	prev = dynmap_set(struct_union_enum_st *, type *, root->enumints, enu, ent);
	assert(!prev);

	return ent;
}

type *type_unqualify(type *t)
{
	type *t_restrict = NULL, *prev = NULL;

	while(t){
		if(t->type == type_cast && !t->bits.cast.is_signed_cast){
			/* restrict qualifier is special, and is only on pointer
			 * types and doesn't really apply to the expression itself
			 */
			if(t->bits.cast.qual & qual_restrict)
				t_restrict = t;

			prev = t;
			t = t->ref;
		}else{
			break;
		}
	}

	if(t_restrict){
		assert(prev);
		if(prev == t_restrict){
			/* fine - we can just return this, preserving restrictness,
			 * as nothing below it is a qualifier */
			return t_restrict;
		}else{
			/* preserve restrict */
			return type_qualify(t, qual_restrict);
		}
	}

	return t;
}

type *type_at_where(type *t, where *w)
{
	if(t->type != type_where || !where_equal(w, &t->bits.where)){
		t = type_new(type_where, t);
		memcpy_safe(&t->bits.where, w);
	}
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

type *type_nav_auto(struct type_nav *root)
{
	if(!root->tauto)
		root->tauto = type_new(type_auto, NULL);

	return root->tauto;
}

type *type_nav_suetype(struct type_nav *root, struct_union_enum_st *sue)
{
	type *ent, *prev;
	btype *bt;

	if(!root->suetypes){
		root->suetypes = dynmap_new(
				struct_union_enum_st *, /*refeq:*/NULL, sue_hash);
	}

	ent = dynmap_get(struct_union_enum_st *, type *, root->suetypes, sue);

	if(ent)
		return ent;

	bt = umalloc(sizeof *bt);
	bt->primitive = sue->primitive;
	bt->sue = sue;
	ent = type_new_btype(bt);

	prev = dynmap_set(struct_union_enum_st *, type *, root->suetypes, sue, ent);
	assert(!prev);

	return ent;
}

type *type_nav_va_list(struct type_nav *root, symtable *symtab)
{
	if(!root->tva_list)
		root->tva_list = c_types_make_va_list(symtab);

	return root->tva_list;
}

type *type_nav_voidptr(struct type_nav *root)
{
    return type_ptr_to(type_nav_btype(root, type_void));
}

type *type_nav_changeauto(type *const ontop, type *trailing)
{
	type *base;

	if(!ontop || ontop->type == type_btype)
		return trailing; /* replace auto with proper trailing type */

	/* use recursion to pop non-btypes on top of trailing */
	base = type_nav_changeauto(type_next_1(ontop), trailing);

	/* pop our type on top of trailing */
	switch(ontop->type){
		case type_btype:
		case type_auto:
			assert(0);

		case type_ptr:
			return type_ptr_to(base);

		case type_block:
			return type_block_of(base);

		case type_array:
		{
			return type_array_of_static(
					base,
					ontop->bits.array.size,
					ontop->bits.array.is_static);
		}

		case type_func:
		{
			funcargs *args = type_funcargs(ontop);
			args->retains++;

			return type_func_of(
					base, args,
					ontop->bits.func.arg_scope);
		}

		case type_tdef:
		case type_cast:
			return base;

		case type_where:
			return type_at_where(base, &ontop->bits.where);

		case type_attr:
			return type_attributed(base, ontop->bits.attr);
	}

	assert(0);
}

static void type_dump_t(type *t, FILE *f, int indent)
{
	int i;
	for(i = 0; i < indent; i++)
		fputc(' ', f);

	fprintf(f, "%s %s%s\n",
			type_kind_to_str(t->type),
			type_to_str(t),
			t->type == type_ptr && t->bits.ptr.decayed_from
				? " [decayed]" : "");

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
	type *t;

	for(i = 0; i < N_TYPE_KINDS; i++){
		t = nav->btypes[i];
		if(t)
			type_dump_t(t, stderr, 0);
	}

	for(i = 0;
	    (t = dynmap_value(type *, nav->suetypes, i));
			i++)
	{
		type_dump_t(t, stderr, 0);
	}
}
