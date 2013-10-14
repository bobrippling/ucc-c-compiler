#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/platform.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "macros.h"
#include "sue.h"
#include "const.h"
#include "cc1.h"
#include "fold.h"
#include "funcargs.h"

#define ITER_DESC_TYPE(d, dp, typ)     \
	for(dp = d->desc; dp; dp = dp->child) \
		if(dp->type == typ)


static type_ref *type_ref_new(enum type_ref_type t, type_ref *of)
{
	type_ref *r = umalloc(sizeof *r);
	if(of)
		memcpy_safe(&r->where, &of->where);
	else
		where_cc1_current(&r->where);

	r->type = t;
	r->ref = of;
	return r;
}

static type_ref *cache_basics[type_unknown];
static type_ref *cache_ptr[type_unknown];
static type_ref *cache_va_list;

void type_ref_init(symtable *stab)
{
	const where w = WHERE_INIT("<builtin>", "<builtin>", 1, 1);
	eof_where = &w;

	/* FIXME: cache unsigned types too */
	cache_basics[type_void] = type_ref_cached_VOID();
	cache_basics[type_int]  = type_ref_cached_INT();
	cache_basics[type_long] = type_ref_cached_INTPTR_T();
	cache_basics[type_llong] = type_ref_cached_LLONG();
	cache_basics[type_double] = type_ref_cached_DOUBLE();

	cache_basics[type_schar] = type_ref_cached_CHAR(s);
	cache_basics[type_uchar] = type_ref_cached_CHAR(u);
	cache_basics[type_nchar] = type_ref_cached_CHAR(n);

	cache_ptr[type_void] = type_ref_cached_VOID_PTR();
	cache_ptr[type_long] = type_ref_cached_LONG_PTR();
	cache_ptr[type_int]  = type_ref_cached_INT_PTR();

	/* pointer to struct __builtin_va_list */
	{
		/* must match platform abi - vfprintf(..., ap); */
		sue_member **sue_members = NULL;

		type_ref *void_ptr = type_ref_cached_VOID_PTR();

		/*
		unsigned int gp_offset;
		unsigned int fp_offset;
		void *overflow_arg_area;
		void *reg_save_area;
		*/

#define ADD_DECL(to, dcl)          \
		dynarray_add(&to,              \
				sue_member_from_decl(dcl))

#define ADD_SCALAR(to, ty, sp)       \
		ADD_DECL(to,                     \
				decl_new_ty_sp(              \
					type_ref_new_type(         \
						type_new_primitive(ty)), \
					ustrdup(sp)))


		ADD_SCALAR(sue_members, type_int, "gp_offset");
		ADD_SCALAR(sue_members, type_int, "fp_offset");
		ADD_DECL(sue_members, decl_new_ty_sp(void_ptr, "overflow_arg_area"));
		ADD_DECL(sue_members, decl_new_ty_sp(void_ptr, "reg_save_area"));

		/* typedef struct __va_list_struct __builtin_va_list[1]; */
		{
			type_ref *va_list_struct = type_ref_new_type(
					type_new_primitive_sue(
						type_struct,
						sue_decl(stab, ustrdup("__va_list_struct"),
							sue_members, type_struct, 1, 1)));


			type_ref *builtin_ar = type_ref_new_array(
					va_list_struct,
					expr_new_val(1));

			type_ref *td = type_ref_new_tdef(
					expr_new_sizeof_type(builtin_ar, 1),
					decl_new_ty_sp(builtin_ar,
						ustrdup("__builtin_va_list")));

			cache_va_list = td;
		}
	}

	eof_where = NULL;
}

type_ref *type_ref_cached_VA_LIST(void)
{
	return cache_va_list;
}

type_ref *type_ref_cached_VA_LIST_decayed(void)
{
	static type_ref *cache_va_list_decayed;

	if(!cache_va_list_decayed)
		cache_va_list_decayed = type_ref_decay(
				type_ref_cached_VA_LIST());

	return cache_va_list_decayed;
}

type_ref *type_ref_new_type(const type *t)
{
	type_ref *r = type_ref_new(type_ref_type, NULL);
	r->bits.type = t;
	return r;
}

type_ref *type_ref_new_type_primitive(enum type_primitive p)
{
	type_ref *r;
	if((r = cache_basics[p]))
		return r;
	return type_ref_new_type(type_new_primitive(p));
}

type_ref *type_ref_new_type_qual(enum type_primitive t, enum type_qualifier q)
{
	return type_ref_new_cast(
			type_ref_new_type(type_new_primitive(t)),
			q);
}

type_ref *type_ref_new_tdef(expr *e, decl *to)
{
	type_ref *r = type_ref_new(type_ref_tdef, NULL);
	UCC_ASSERT(expr_kind(e, sizeof), "not sizeof for tdef ref");
	r->bits.tdef.type_of = e;
	r->bits.tdef.decl = to; /* NULL if typeof */
	return r;
}

type_ref *type_ref_new_ptr(type_ref *to, enum type_qualifier q)
{
	type_ref *r = type_ref_new(type_ref_ptr, to);
	r->bits.ptr.qual = q;
	return r;
}

type_ref *type_ref_new_block(type_ref *to, enum type_qualifier q)
{
	type_ref *r = type_ref_new_ptr(to, q);
	r->type = type_ref_block;
	return r;
}

type_ref *type_ref_new_array(type_ref *to, expr *sz)
{
	type_ref *r = type_ref_new(type_ref_array, to);
	r->bits.array.size = sz;
	return r;
}

type_ref *type_ref_new_array2(type_ref *to, expr *sz,
		enum type_qualifier q, int is_static)
{
	type_ref *r = type_ref_new_array(to, sz);
	r->bits.array.is_static = is_static;
	r->bits.array.qual      = q;
	return r;
}

type_ref *type_ref_new_func(type_ref *of, funcargs *args)
{
	type_ref *r = type_ref_new(type_ref_func, of);
	r->bits.func.args = args;
	r->bits.func.arg_scope = current_scope;
	return r;
}

type_ref *type_ref_cached_MAX_FOR(unsigned sz)
{
	enum type_primitive prims[] = {
		type_long, type_int, type_short, type_nchar
	};
	unsigned i;

	for(i = 0; i < sizeof(prims)/sizeof(*prims); i++)
		if(sz >= type_primitive_size(prims[i]))
			return type_ref_new_type(type_new_primitive(prims[i]));
	return NULL;
}

static type_ref *type_ref_new_cast_is_additive(type_ref *to, enum type_qualifier new, int additive)
{
	type_ref *r;

	if(!new)
		return to;

	r = type_ref_new(type_ref_cast, to);
	r->bits.cast.qual = new;
	r->bits.cast.additive = additive;
	return r;
}

type_ref *type_ref_new_cast(type_ref *to, enum type_qualifier new)
{
	return type_ref_new_cast_is_additive(to, new, 0);
}

type_ref *type_ref_new_cast_add(type_ref *to, enum type_qualifier add)
{
	return type_ref_new_cast_is_additive(to, add, 1);
}

type_ref *type_ref_new_cast_signed(type_ref *to, int is_signed)
{
	type_ref *r = type_ref_new(type_ref_cast, to);

	r->bits.cast.is_signed_cast = 1;
	r->bits.cast.signed_true = is_signed;

	return r;
}

decl *decl_new_w(const where *w)
{
	decl *d = umalloc(sizeof *d);
	memcpy_safe(&d->where, w);
	return d;
}

decl *decl_new()
{
	where wtmp;
	where_cc1_current(&wtmp);
	return decl_new_w(&wtmp);
}

decl *decl_new_ty_sp(type_ref *ty, char *sp)
{
	decl *d = decl_new();
	d->ref = ty;
	d->spel = sp;
	return d;
}

void decl_replace_with(decl *to, decl *from)
{
	/* XXX: memleak of .ref */
	memcpy_safe(&to->where, &from->where);
	to->ref      = from->ref;
	to->attr     = from->attr;
	to->spel_asm = from->spel_asm;
	/* no point copying bitfield stuff */
	to->align    = from->align;
}

const char *decl_asm_spel(decl *d)
{
	if(!d->spel_asm){
		/* apply underscore prefixes, name mangling, etc */
		type_ref *rf = DECL_IS_FUNC(d);
		char *pre, suff[8];

		pre = fopt_mode & FOPT_LEADING_UNDERSCORE ? "_" : "";
		*suff = '\0';

		if(rf){
			funcargs *fa = type_ref_funcargs(rf);

			switch(fa->conv){
				case conv_fastcall:
					pre = "@";

				case conv_stdcall:
					snprintf(suff, sizeof suff,
							"@%d",
							dynarray_count(fa->arglist) * platform_word_size());

				case conv_x64_sysv:
				case conv_x64_ms:
				case conv_cdecl:
					break;
			}
		}

		if(*pre || *suff)
			d->spel_asm = ustrprintf(
					"%s%s%s", pre, d->spel, suff);


		if(!d->spel_asm)
			d->spel_asm = d->spel;
	}

	return d->spel_asm;
}

void type_ref_free_1(type_ref *r)
{
	if(!r)
		return;

	switch(r->type){
		case type_ref_type:
			/* XXX: memleak */
			/*type_free(r->bits.type);*/
			if(r == cache_basics[r->bits.type->primitive])
				return; /* don't free the cache */
			break;

		case type_ref_func:
			/* XXX: memleak x2 */
			funcargs_free(r->bits.func.args, 1, 0);
			break;
		case type_ref_block:
			funcargs_free(r->bits.block.func, 1, 0);
			break;

		case type_ref_array:
			expr_free(r->bits.array.size);
			break;

		case type_ref_cast:
		case type_ref_ptr:
		case type_ref_tdef:
			break;
	}

	decl_attr_free(r->attr);

	free(r);
}

void type_ref_free(type_ref *r)
{
	if(!r)
		return;

	type_ref_free(r->ref);

	type_ref_free_1(r);
}

void decl_free(decl *d, int free_ref)
{
	if(!d)
		return;

	if(free_ref)
		type_ref_free(d->ref);

	expr_free(d->field_width); /* XXX: bad? */

	free(d);
}

decl_attr *decl_attr_new(enum decl_attr_type t)
{
	decl_attr *da = umalloc(sizeof *da);
	where_cc1_current(&da->where);
	da->type = t;
	return da;
}

void decl_attr_append(decl_attr **loc, decl_attr *new)
{
	/* may be appending from a prototype to a function def. */
	while(*loc)
		loc = &(*loc)->next;

	/* we can just link up, since pointers aren't rewritten now */
	*loc = /*decl_attr_copy(*/new/*)*/;
}

decl_attr *attr_present(decl_attr *da, enum decl_attr_type t)
{
	for(; da; da = da->next)
		if(da->type == t)
			return da;
	return NULL;
}

decl_attr *type_attr_present(type_ref *r, enum decl_attr_type t)
{
	/*
	 * attributes can be on:
	 *
	 * decl (spel)
	 * type_ref (specifically the type, pointer or func, etc)
	 * sue (struct A {} __attribute((packed)))
	 * type (__attribute((section("data"))) int a)
	 *
	 * this means typedefs carry attributes too
	 */

	while(r){
		decl_attr *da;

		if((da = attr_present(r->attr, t)))
			return da;

		switch(r->type){
			case type_ref_type:
			{
				struct_union_enum_st *sue = r->bits.type->sue;
				if((da = attr_present(r->bits.type->attr, t)))
					return da;
				return sue ? attr_present(sue->attr, t) : NULL;
			}

			case type_ref_tdef:
			{
				decl *d = r->bits.tdef.decl;

				if(d && (da = attr_present(d->attr, t)))
					return da;

				return expr_attr_present(r->bits.tdef.type_of, t);
			}

			case type_ref_ptr:
			case type_ref_block:
			case type_ref_func:
			case type_ref_array:
			case type_ref_cast:
				r = r->ref;
				break;
		}
	}
	return NULL;
}

decl_attr *decl_attr_present(decl *d, enum decl_attr_type t)
{
	/* check the attr on the decl _and_ its type */
	decl_attr *da;
	if((da = attr_present(d->attr, t)))
		return da;
	if((da = type_attr_present(d->ref, t)))
		return da;

	return d->proto ? decl_attr_present(d->proto, t) : NULL;
}

decl_attr *expr_attr_present(expr *e, enum decl_attr_type t)
{
	decl_attr *da;

	if(expr_kind(e, cast)){
		da = expr_attr_present(e->expr, t);
		if(da)
			return da;
	}

	if(expr_kind(e, identifier)){
		sym *s = e->bits.ident.sym;
		if(s){
			da = decl_attr_present(s->decl, t);
			if(da)
				return da;
		}
	}

	return type_attr_present(e->tree_type, t);
}

const char *decl_attr_to_str(decl_attr *da)
{
	switch(da->type){
		CASE_STR_PREFIX(attr, format);
		CASE_STR_PREFIX(attr, unused);
		CASE_STR_PREFIX(attr, warn_unused);
		CASE_STR_PREFIX(attr, section);
		CASE_STR_PREFIX(attr, enum_bitmask);
		CASE_STR_PREFIX(attr, noreturn);
		CASE_STR_PREFIX(attr, noderef);
		CASE_STR_PREFIX(attr, nonnull);
		CASE_STR_PREFIX(attr, packed);
		CASE_STR_PREFIX(attr, sentinel);
		CASE_STR_PREFIX(attr, aligned);

		case attr_call_conv:
			switch(da->bits.conv){
				case conv_x64_sysv: return "x64 SYSV";
				case conv_x64_ms:   return "x64 MS";
				CASE_STR_PREFIX(conv, cdecl);
				CASE_STR_PREFIX(conv, stdcall);
				CASE_STR_PREFIX(conv, fastcall);
			}

		case attr_LAST:
			break;
	}
	return NULL;
}

const char *decl_store_to_str(const enum decl_storage s)
{
	static char buf[16]; /* "inline register" is the longest - just a fit */

	if(s & STORE_MASK_EXTRA){
		*buf = '\0';

		if((s & STORE_MASK_EXTRA) == store_inline)
			strcpy(buf, "inline ");

		strcpy(buf + strlen(buf), decl_store_to_str(s & STORE_MASK_STORE));
		return buf;
	}

	switch(s){
		case store_inline:
			ICE("inline");
		case store_default:
			return "";
		CASE_STR_PREFIX(store, auto);
		CASE_STR_PREFIX(store, static);
		CASE_STR_PREFIX(store, extern);
		CASE_STR_PREFIX(store, register);
		CASE_STR_PREFIX(store, typedef);
	}
	return NULL;
}

void decl_attr_free(decl_attr *a)
{
	if(!a)
		return;

	decl_attr_free(a->next);
	free(a);
}

unsigned type_ref_size(type_ref *r, where *from)
{
	switch(r->type){
		case type_ref_type:
			return type_size(r->bits.type, from);

		case type_ref_tdef:
		{
			decl *d = r->bits.tdef.decl;
			type_ref *sub;

			if(d)
				return type_ref_size(d->ref, from);

			sub = r->bits.tdef.type_of->tree_type;
			UCC_ASSERT(sub, "type_ref_size for unfolded typedef");
			return type_ref_size(sub, from);
		}

		case type_ref_cast:
			return type_ref_size(r->ref, from);

		case type_ref_ptr:
		case type_ref_block:
			return platform_word_size();

		case type_ref_func:
			/* function size is one, sizeof(main) is valid */
			return 1;

		case type_ref_array:
		{
			integral_t sz;

			if(type_ref_is_void(r->ref))
				die_at(from, "array of void");

			if(!r->bits.array.size)
				die_at(from, "array has an incomplete size");

			sz = const_fold_val_i(r->bits.array.size);

			return sz * type_ref_size(r->ref, from);
		}
	}

	ucc_unreach(0);
}

unsigned decl_size(decl *d)
{
	if(type_ref_is_void(d->ref))
		die_at(&d->where, "%s is void", d->spel);

	if(d->field_width)
		die_at(&d->where, "can't take size of a bitfield");

	return type_ref_size(d->ref, &d->where);
}

unsigned decl_align(decl *d)
{
	unsigned al = 0;

	if(d->align)
		al = d->align->resolved;

	return al ? al : type_ref_align(d->ref, &d->where);
}

enum type_cmp decl_cmp(decl *a, decl *b, enum type_cmp_opts opts)
{
	enum type_cmp cmp = type_ref_cmp(a->ref, b->ref, opts);
	enum decl_storage sa = a->store & STORE_MASK_STORE,
	                  sb = b->store & STORE_MASK_STORE;

	if(cmp == TYPE_EQUAL && sa != sb){
		/* types are equal but there's a store mismatch
		 * only return convertible if it's a typedef or static mismatch
		 */
#define STORE_INCOMPAT(st) ((st) == store_typedef || (st) == store_static)

		if(STORE_INCOMPAT(sa) || STORE_INCOMPAT(sb))
			return TYPE_CONVERTIBLE_IMPLICIT;
	}

	return cmp;
}

int decl_sort_cmp(const decl **pa, const decl **pb)
{
	const decl *const a = *pa, *const b = *pb;
	int cmp = strcmp(a->spel, b->spel);

	if(cmp != 0)
		return cmp; /* names take priority */

	/* assume functions - sort by whether they have code */
	return !!a->func_code - !!b->func_code;
}

int type_ref_is_variadic_func(type_ref *r)
{
	return (r = type_ref_is(r, type_ref_func)) && r->bits.func.args->variadic;
}

type_ref *type_ref_ptr_depth_dec(type_ref *r, where *w)
{
	type_ref *const r_save = r;

	r = type_ref_is_ptr(r);

	if(!r){
		die_at(w,
				"invalid indirection applied to %s",
				r_save ? type_ref_to_str(r_save) : "(NULL)");
	}

	/* *(void (*)()) does nothing */
	if(type_ref_is(r, type_ref_func))
		return r_save;

	/* don't check for incomplete types here */

	/* XXX: memleak */
	/*type_ref_free(r_save);*/

	return r;
}

type_ref *type_ref_ptr_depth_inc(type_ref *r)
{
	type_ref *test;
	if((test = type_ref_is_type(r, type_unknown))){
		type_ref *p = cache_ptr[test->bits.type->primitive];
		if(p)
			return p;
	}

	return type_ref_new_ptr(r, qual_none);
}

int decl_conv_array_func_to_ptr(decl *d)
{
	type_ref *old = d->ref;

	d->ref = type_ref_decay(d->ref);

	return old != d->ref;
}

type_ref *type_ref_is_decayed_array(type_ref *r)
{
	if((r = type_ref_is(r, type_ref_ptr)) && r->bits.ptr.size)
		return r;

	return NULL;
}

type_ref *decl_is_decayed_array(decl *d)
{
	return type_ref_is_decayed_array(d->ref);
}

static void type_ref_add_str(type_ref *r, char *spel, int need_spc, char **bufp, int sz)
{
#define BUF_ADD(...) \
	do{ int n = snprintf(*bufp, sz, __VA_ARGS__); *bufp += n, sz -= n; }while(0)
#define ADD_SPC() do{ if(need_spc) BUF_ADD(" "); need_spc = 0; }while(0)

	int need_paren;
	enum type_qualifier q;

	if(!r){
		/* reached the bottom/end - spel */
		if(spel){
			ADD_SPC();
			BUF_ADD("%s", spel);
		}
		return;
	}

	q = qual_none;
	switch(r->ref->type){
		case type_ref_type:
		case type_ref_tdef: /* just starting */
		case type_ref_cast: /* no need */
			need_paren = 0;
			break;

		default:
			/* for now. can be altered */
			need_paren = !r->tmp || r->type != r->tmp->type;
	}

	if(need_paren){
		ADD_SPC();
		BUF_ADD("(");
	}

	switch(r->type){
		case type_ref_ptr:
#ifdef SHOW_DECAYED_ARRAYS
			if(r->bits.ptr.size)
				break; /* decayed array */
#endif

			ADD_SPC();
			BUF_ADD("*");
			q = r->bits.ptr.qual;
			break;

		case type_ref_cast:
			if(r->bits.cast.is_signed_cast){
				ADD_SPC();
				BUF_ADD(r->bits.cast.signed_true ? "signed" : "unsigned");
			}else{
				q = r->bits.cast.qual;
			}
			break;

		case type_ref_block:
			ADD_SPC();
			BUF_ADD("^");
			q = r->bits.block.qual;
			break;

		default:break;
	}

	if(q){
		ADD_SPC();
		BUF_ADD("%s", type_qual_to_str(q, 0));
		need_spc = 1;
		/* space out after qualifier, e.g.
		 * int *const p;
		 *           ^
		 * int const a;
		 *          ^
		 */
	}

	type_ref_add_str(r->tmp, spel, need_spc, bufp, sz);
	need_spc = 0; /* after the spel, no more spaces */

	switch(r->type){
		case type_ref_tdef:
			/* tdef "aka: %s" handled elsewhere */
		case type_ref_type:
		case type_ref_cast:
			/**/
		case type_ref_block:
			break;

		case type_ref_func:
		{
			const char *comma = "";
			decl **i;
			funcargs *args = r->bits.func.args;

			BUF_ADD("(");
			for(i = args->arglist; i && *i; i++){
				char tmp_buf[DECL_STATIC_BUFSIZ];
				BUF_ADD("%s%s", comma, decl_to_str_r(tmp_buf, *i));
				comma = ", ";
			}
			BUF_ADD("%s)", args->variadic ? ", ..." : args->args_void ? "void" : "");
			break;
		}
		case type_ref_ptr:
#ifdef SHOW_DECAYED_ARRAYS
			if(!r->bits.ptr.size)
#endif
				break;
			/* fall */
		case type_ref_array:
			BUF_ADD("[");
			if(r->bits.array.size){
				int spc = 0;

				if(r->bits.array.is_static){
					BUF_ADD("static");
					spc = 1;
				}

				if(r->bits.array.qual){
					BUF_ADD(
							"%s%s",
							spc ? " " : "",
							type_qual_to_str(r->bits.array.qual, 0));
					spc = 1;
				}

				BUF_ADD(
						"%s%" NUMERIC_FMT_D,
						spc ? " " : "",
						const_fold_val_i(r->bits.array.size));
			}
			BUF_ADD("]");

			break;
	}

	if(need_paren)
		BUF_ADD(")");
}

static type_ref *type_ref_set_parent(type_ref *r, type_ref *parent)
{
	if(!r)
		return parent;

	r->tmp = parent;

	return type_ref_set_parent(r->ref, r);
}

static
const char *type_ref_to_str_r_spel_aka(
		char buf[TYPE_REF_STATIC_BUFSIZ], type_ref *r,
		char *spel, const int aka);

static
void type_ref_add_type_str(type_ref *r,
		char **bufp, int sz,
		const int aka)
{
	/* go down to the first type or typedef, print it and then its descriptions */
	const type_ref *rt;

	**bufp = '\0';
	for(rt = r; rt && rt->type != type_ref_type && rt->type != type_ref_tdef; rt = rt->ref);

	if(!rt)
		return;

	if(rt->type == type_ref_tdef){
		char buf[TYPE_REF_STATIC_BUFSIZ];
		decl *d = rt->bits.tdef.decl;
		type_ref *of;

		if(d){
			BUF_ADD("%s", d->spel);
			of = d->ref;

		}else{
			expr *const e = rt->bits.tdef.type_of;
			int const is_type = !e->expr;

			BUF_ADD("typeof(%s%s)",
					/* e is always expr_sizeof() */
					is_type ? "" : "expr: ",
					is_type ? type_ref_to_str_r_spel_aka(buf, e->tree_type, NULL, 0)
						: e->expr->f_str());

			/* don't show aka for typeof types - it's there already */
			of = is_type ? NULL : e->tree_type;
		}

		if(aka && of){
			/* descend to the type if it's next */
			type_ref *t_ref = type_ref_is_type(of, type_unknown);
			const type *t = t_ref ? t_ref->bits.type : NULL;

			BUF_ADD(" (aka '%s')",
					t ? type_to_str(t)
					: type_ref_to_str_r_spel_aka(buf, of, NULL, 0));
		}

	}else{
		BUF_ADD("%s", type_to_str(rt->bits.type));
	}
}
#undef BUF_ADD

int decl_store_static_or_extern(enum decl_storage s)
{
	switch((enum decl_storage)(s & STORE_MASK_STORE)){
		case store_static:
		case store_extern:
			return 1;
		default:
			return 0;
	}
}

static
const char *type_ref_to_str_r_spel_aka(
		char buf[TYPE_REF_STATIC_BUFSIZ], type_ref *r,
		char *spel, const int aka)
{
	char *bufp = buf;

	type_ref_add_type_str(r, &bufp, TYPE_REF_STATIC_BUFSIZ, aka);

	/* print in reverse order */
	r = type_ref_set_parent(r, NULL);
	/* use r->tmp, since r is type_ref_t{ype,def} */
	type_ref_add_str(r->tmp, spel, 1,
			&bufp, TYPE_REF_STATIC_BUFSIZ - (bufp - buf));

	/* trim trailing space */
	if(bufp > buf && bufp[-1] == ' ')
		bufp[-1] = '\0';

	return buf;
}

const char *type_ref_to_str_r_spel(char buf[TYPE_REF_STATIC_BUFSIZ], type_ref *r, char *spel)
{
	return type_ref_to_str_r_spel_aka(buf, r, spel, 1);
}

const char *type_ref_to_str_r(char buf[TYPE_REF_STATIC_BUFSIZ], type_ref *r)
{
	return type_ref_to_str_r_spel(buf, r, NULL);
}

const char *type_ref_to_str_r_show_decayed(char buf[TYPE_REF_STATIC_BUFSIZ], type_ref *r)
{
	const char *s;
	r->type = type_ref_array;
	s = type_ref_to_str_r(buf, r);
	r->type = type_ref_ptr;
	return s;
}

const char *type_ref_to_str(type_ref *r)
{
	static char buf[TYPE_REF_STATIC_BUFSIZ];
	return type_ref_to_str_r(buf, r);
}

const char *decl_to_str_r(char buf[DECL_STATIC_BUFSIZ], decl *d)
{
	char *bufp = buf;

	if(d->store)
		bufp += snprintf(bufp, DECL_STATIC_BUFSIZ, "%s ", decl_store_to_str(d->store));

	type_ref_to_str_r_spel(bufp, d->ref, d->spel);

	return buf;
}

const char *decl_to_str(decl *d)
{
	static char buf[DECL_STATIC_BUFSIZ];
	return decl_to_str_r(buf, d);
}
