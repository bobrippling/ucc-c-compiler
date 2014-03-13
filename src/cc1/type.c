#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/util.h"
#include "../util/platform.h"

#include "macros.h"

#include "expr.h"
#include "sue.h"
#include "type.h"
#include "decl.h"
#include "const.h"
#include "funcargs.h"
#include "cc1.h" /* fopt_mode */
#include "defs.h"

#include "type_is.h"

static int type_qual_cmp_1(
		enum type_qualifier a,
		enum type_qualifier b,
		enum type_qualifier mask)
{
	switch(!!(a & mask) - !!(b & mask)){
		case 0:
			return 0;
		case 1: /* a has it */
			return -1;
		case -1: /* b has it */
			return 1;
	}
	ucc_unreach(0);
}

static int type_qual_cmp(enum type_qualifier a, enum type_qualifier b)
{
	int r;

	a &= ~qual_restrict,
	b &= ~qual_restrict;

	if(a == b)
		return 0;

	/* check const, then volatile */
	r = type_qual_cmp_1(a, b, qual_const);
	if(r)
		return r;

	r = type_qual_cmp_1(a, b, qual_volatile);
	if(r)
		return r;

	/* should be unreachable, or we've missed a qual_* */
	return 0;
}

static enum type_cmp type_cmp_r(
		type *const orig_a,
		type *const orig_b,
		enum type_cmp_opts opts)
{
	enum type_cmp ret;
	type *a, *b;
	int subchk = 1;

	if(!orig_a || !orig_b)
		return orig_a == orig_b ? TYPE_EQUAL : TYPE_NOT_EQUAL;

	a = type_skip_all(orig_a);
	b = type_skip_all(orig_b);

	/* array/func decay takes care of any array->ptr checks */
	if(a->type != b->type){
		/* allow _Bool <- pointer */
		if(type_is_primitive(a, type__Bool) && type_is_ptr(b))
			return TYPE_CONVERTIBLE_IMPLICIT;

		/* allow int <-> ptr */
		if((type_is_ptr(a) && type_is_integral(b))
		|| (type_is_ptr(b) && type_is_integral(a)))
		{
			return TYPE_CONVERTIBLE_EXPLICIT;
		}

		/* allow void <- anything */
		if(type_is_void(a))
			return TYPE_CONVERTIBLE_IMPLICIT;

		/* allow block <-> fnptr */
		if((type_is_fptr(a) && type_is(b, type_block))
		|| (type_is_fptr(b) && type_is(a, type_block)))
		{
			return TYPE_CONVERTIBLE_EXPLICIT;
		}

		return TYPE_NOT_EQUAL;
	}

	switch(a->type){
		case type_btype:
			subchk = 0;
			ret = btype_cmp(a->bits.type, b->bits.type);
			break;

		case type_array:
		{
			const int a_complete = !!a->bits.array.size,
			          b_complete = !!b->bits.array.size;

			if(a_complete && b_complete){
				const integral_t av = const_fold_val_i(a->bits.array.size),
				                 bv = const_fold_val_i(b->bits.array.size);

				if(av != bv)
					return TYPE_NOT_EQUAL;
			}else if(a_complete != b_complete){
				if((opts & TYPE_CMP_ALLOW_TENATIVE_ARRAY) == 0)
					return TYPE_NOT_EQUAL;
			}

			/* next */
			break;
		}

		case type_block:
		case type_ptr:
			break;

		case type_cast:
		case type_tdef:
		case type_attr:
		case type_where:
			ICE("should've been skipped");

		case type_func:
			switch(funcargs_cmp(a->bits.func.args, b->bits.func.args)){
				case FUNCARGS_EXACT_EQUAL:
				case FUNCARGS_IMPLICIT_CONV:
					break;
				default:
					/* "void (int)" and "void (int, int)" aren't equal,
					 * but a cast can soon fix it */
					return TYPE_CONVERTIBLE_EXPLICIT;
			}
			break;
	}

	if(subchk)
		ret = type_cmp_r(a->ref, b->ref, opts);

	if(ret == TYPE_NOT_EQUAL
	&& a->type == type_func)
	{
		/* "int (int)" and "void (int)" aren't equal - but castable */
		ret = TYPE_CONVERTIBLE_EXPLICIT;
	}

	if(ret == TYPE_NOT_EQUAL
	&& a->type == type_ptr
	&& fopt_mode & FOPT_PLAN9_EXTENSIONS)
	{
		/* allow b to be an anonymous member of a, if pointers */
		struct_union_enum_st *a_sue = type_is_s_or_u(a),
		                     *b_sue = type_is_s_or_u(b);

		if(a_sue && b_sue /* already know they aren't equal */){
			/* b_sue has an a_sue,
			 * the implicit cast adjusts to return said a_sue */
			if(struct_union_member_find_sue(b_sue, a_sue))
				return TYPE_CONVERTIBLE_IMPLICIT;
		}
	}

	/* allow ptr <-> ptr */
	if(ret == TYPE_NOT_EQUAL && type_is_ptr(a) && type_is_ptr(b))
		ret = TYPE_CONVERTIBLE_EXPLICIT;

	/* char * and int * are explicitly conv.,
	 * even though char and int are implicit */
	if(ret == TYPE_CONVERTIBLE_IMPLICIT && a->type == type_ptr)
		ret = TYPE_CONVERTIBLE_EXPLICIT;

	if(a->type == type_ptr || a->type == type_block){
		switch(ret){
#define MAP(a, b) case a: ret = b; break
			MAP(TYPE_QUAL_ADD, TYPE_QUAL_POINTED_ADD);
			MAP(TYPE_QUAL_SUB, TYPE_QUAL_POINTED_SUB);
			MAP(TYPE_QUAL_POINTED_ADD, TYPE_QUAL_NESTED_CHANGE);
			MAP(TYPE_QUAL_POINTED_SUB, TYPE_QUAL_NESTED_CHANGE);
#undef MAP
			default:
				break;
		}
	}

	if(ret & TYPE_EQUAL_ANY){
		enum type_qualifier a_qual = type_qual(orig_a);
		enum type_qualifier b_qual = type_qual(orig_b);

		if(a_qual && b_qual){
			switch(type_qual_cmp(a_qual, b_qual)){
				case -1:
					/* a has more */
					ret = TYPE_QUAL_ADD;
					break;
				case 1:
					/* b has more */
					ret = TYPE_QUAL_SUB;
					break;
			}
		}else if(a_qual){
			ret = TYPE_QUAL_ADD;
		}else if(b_qual){
			ret = TYPE_QUAL_SUB;
		} /* else neither are casts */
	}

	if(ret == TYPE_EQUAL){
		int at = orig_a->type == type_tdef;
		int bt = orig_b->type == type_tdef;

		if(at != bt){
			/* one is a typedef */
			ret = TYPE_EQUAL_TYPEDEF;
		}else if(at){
			/* both typedefs */
			if(orig_a->bits.tdef.decl != orig_b->bits.tdef.decl){
				ret = TYPE_EQUAL_TYPEDEF;
			}
		}
		/* else no typedefs */
	}

	return ret;
}

enum type_cmp type_cmp(type *a, type *b, enum type_cmp_opts opts)
{
	const enum type_cmp cmp = type_cmp_r(a, b, opts);

	if(cmp == TYPE_CONVERTIBLE_EXPLICIT){
		/* try for implicit void * conversion */
		if(type_is_void_ptr(a) && type_is_ptr(b))
			return TYPE_CONVERTIBLE_IMPLICIT;

		if(type_is_void_ptr(b) && type_is_ptr(a))
			return TYPE_CONVERTIBLE_IMPLICIT;
	}

	return cmp;
}

integral_t type_max(type *r, where *from)
{
	unsigned sz = type_size(r, from);

	return 1ULL << (sz * CHAR_BIT - 1);
}

unsigned type_size(type *r, where *from)
{
	switch(r->type){
		case type_btype:
			return btype_size(r->bits.type, from);

		case type_tdef:
		{
			decl *d = r->bits.tdef.decl;
			type *sub;

			if(d)
				return type_size(d->ref, from);

			sub = r->bits.tdef.type_of->tree_type;
			UCC_ASSERT(sub, "type_size for unfolded typedef");
			return type_size(sub, from);
		}

		case type_attr:
		case type_cast:
		case type_where:
			return type_size(r->ref, from);

		case type_ptr:
		case type_block:
			return platform_word_size();

		case type_func:
			/* function size is one, sizeof(main) is valid */
			return 1;

		case type_array:
		{
			integral_t sz;

			if(type_is_void(r->ref))
				die_at(from, "array of void");

			if(!r->bits.array.size)
				die_at(from, "array has an incomplete size");

			sz = const_fold_val_i(r->bits.array.size);

			return sz * type_size(r->ref, from);
		}
	}

	ucc_unreach(0);
}

unsigned type_align(type *r, where *from)
{
	struct_union_enum_st *sue;
	type *test;

	if((sue = type_is_s_or_u(r)))
		/* safe - can't have an instance without a ->sue */
		return sue->align;

	if(type_is(r, type_ptr)
	|| type_is(r, type_block))
	{
		return platform_word_size();
	}

	if((test = type_is(r, type_btype)))
		return btype_align(test->bits.type, from);

	if((test = type_is(r, type_array)))
		return type_align(test->ref, from);

	return 1;
}

where *type_loc(type *t)
{
	static where fallback;

	t = type_skip_non_wheres(t);
	if(t && t->type == type_where)
		return &t->bits.where;

	if(!fallback.fname)
		fallback.fname = "<unknown>";

	return &fallback;
}

int type_has_loc(type *t)
{
	t = type_skip_non_wheres(t);
	return t && t->type == type_where;
}

static void type_add_str(type *r, char *spel, int *need_spc, char **bufp, int sz)
{
#define BUF_ADD(...) \
	do{ int n = snprintf(*bufp, sz, __VA_ARGS__); *bufp += n, sz -= n; }while(0)
#define ADD_SPC() do{ if(*need_spc) BUF_ADD(" "); *need_spc = 0; }while(0)
#define IS_PTR(ty) ((ty) == type_ptr || (ty) == type_block)

	int need_paren;
	enum type_qualifier q;
	type *prev_skipped;

	if(!r){
		/* reached the bottom/end - spel */
		if(spel){
			ADD_SPC();
			BUF_ADD("%s", spel);
			*need_spc = 0;
		}
		return;
	}

	/* int (**fn())[2]
	 * btype -> array -> ptr -> ptr -> func
	 *                ^ parens
	 *
	 * .tmp looks right, down the chain, .ref looks left, up the chain
	 */
	need_paren = r->ref
		&& IS_PTR(r->type)
		&& (prev_skipped = type_skip_all(r->ref))->type != type_btype
		&& !IS_PTR(prev_skipped->type);

	q = qual_none;

	if(need_paren){
		ADD_SPC();
		BUF_ADD("(");
	}

	switch(r->type){
		case type_ptr:
#ifdef SHOW_DECAYED_ARRAYS
			if(r->bits.ptr.size)
				break; /* decayed array */
#endif

			ADD_SPC();
			BUF_ADD("*");
			break;

		case type_cast:
			if(r->bits.cast.is_signed_cast){
				ADD_SPC();
				BUF_ADD(r->bits.cast.signed_true ? "signed" : "unsigned");
			}else{
				q = r->bits.cast.qual;
			}
			break;

		case type_block:
			ADD_SPC();
			BUF_ADD("^");
			break;

		default:break;
	}

	if(q){
		ADD_SPC();
		BUF_ADD("%s", type_qual_to_str(q, 0));
		*need_spc = 1;
		/* space out after qualifier, e.g.
		 * int *const p;
		 *           ^
		 * int const a;
		 *          ^
		 */
	}

	type_add_str(r->tmp, spel, need_spc, bufp, sz);

	switch(r->type){
		case type_tdef:
			/* tdef "aka: %s" handled elsewhere */
		case type_attr:
			/* attribute not handled here */
		case type_btype:
		case type_cast:
		case type_where:
			/**/
		case type_block:
			break;

		case type_func:
		{
			const char *comma = "";
			decl **i;
			funcargs *args = r->bits.func.args;

			ADD_SPC();
			BUF_ADD("(");
			for(i = args->arglist; i && *i; i++){
				char tmp_buf[DECL_STATIC_BUFSIZ];
				BUF_ADD("%s%s", comma, decl_to_str_r(tmp_buf, *i));
				comma = ", ";
			}
			BUF_ADD("%s)", args->variadic ? ", ..." : args->args_void ? "void" : "");
			break;
		}
		case type_ptr:
#ifdef SHOW_DECAYED_ARRAYS
			if(!r->bits.ptr.size)
#endif
				break;
			/* fall */
		case type_array:
			BUF_ADD("[");
			if(r->bits.array.size){
				int spc = 0;

				if(r->bits.array.is_static){
					BUF_ADD("static");
					spc = 1;
				}

#if 0
				if(r->bits.array.qual){
					BUF_ADD(
							"%s%s",
							spc ? " " : "",
							type_qual_to_str(r->bits.array.qual, 0));
					spc = 1;
				}
#endif

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
#undef IS_PTR
}

static
const char *type_to_str_r_spel_aka(
		char buf[BTYPE_STATIC_BUFSIZ], type *r,
		char *spel, const int aka);

static
type *type_add_type_str(type *r,
		char **bufp, int sz,
		const int aka)
{
	/* go down to the first type or typedef, print it and then its descriptions */
	const type *rt;

	**bufp = '\0';
	for(rt = r;
			rt && rt->type != type_btype && rt->type != type_tdef;
			rt = rt->ref);

	if(!rt)
		return NULL;

	if(rt->type == type_tdef){
		char buf[BTYPE_STATIC_BUFSIZ];
		decl *d = rt->bits.tdef.decl;
		type *of;
		type *next = NULL;

		if(d){
			BUF_ADD("%s", d->spel);
			of = d->ref;
			next = type_next(of);

		}else{
			expr *const e = rt->bits.tdef.type_of;
			int const is_type = !e->expr;

			BUF_ADD("typeof(%s%s)",
					/* e is always expr_sizeof() */
					is_type ? "" : "expr: ",
					is_type ? type_to_str_r_spel_aka(buf, e->tree_type, NULL, 0)
						: e->expr->f_str());

			/* don't show aka for typeof types - it's there already */
			of = is_type ? NULL : e->tree_type;
			next = type_next(e->tree_type);
		}

		if(aka && of){
			/* descend to the type if it's next */
			type *t_ref = type_is_primitive(of, type_unknown);
			const btype *t = t_ref ? t_ref->bits.type : NULL;

			BUF_ADD(" (aka '%s')",
					t ? btype_to_str(t)
					: type_to_str_r_spel_aka(buf, of, NULL, 0));
		}

		return next;

	}else{
		BUF_ADD("%s", btype_to_str(rt->bits.type));
	}

	return NULL;
}
#undef BUF_ADD

static type *type_set_parent(type *r, type *parent)
{
	if(!r)
		return parent;

	r->tmp = parent;

	return type_set_parent(r->ref, r);
}

static
const char *type_to_str_r_spel_aka(
		char buf[TYPE_STATIC_BUFSIZ], type *r,
		char *spel, const int aka)
{
	char *bufp = buf;
	int spc = 1;
	type *skipped;

	skipped = type_add_type_str(r, &bufp, TYPE_STATIC_BUFSIZ, aka);

	if(skipped)
		r = skipped;

	/* print in reverse order */
	r = type_set_parent(r, NULL);
	/* use r->tmp, since r is type_t{ype,def} */
	type_add_str(r->tmp, spel, &spc,
			&bufp, TYPE_STATIC_BUFSIZ - (bufp - buf));

	/* trim trailing space */
	if(bufp > buf && bufp[-1] == ' ')
		bufp[-1] = '\0';

	return buf;
}

const char *type_to_str_r_spel(char buf[TYPE_STATIC_BUFSIZ], type *r, char *spel)
{
	return type_to_str_r_spel_aka(buf, r, spel, 1);
}

const char *type_to_str_r(char buf[TYPE_STATIC_BUFSIZ], type *r)
{
	return type_to_str_r_spel(buf, r, NULL);
}

const char *type_to_str_r_show_decayed(char buf[TYPE_STATIC_BUFSIZ], struct type *r)
{
	const char *s;
	enum type_kind restore;

	r = type_skip_all(r);
	restore = r->type;
	if(r->type == type_ptr)
		r->type = type_array;

	s = type_to_str_r(buf, r);

	r->type = restore;

	return s;
}

const char *type_to_str(type *r)
{
	static char buf[TYPE_STATIC_BUFSIZ];
	return type_to_str_r(buf, r);
}

const char *type_kind_to_str(enum type_kind k)
{
	switch(k){
		CASE_STR_PREFIX(type, btype);
		CASE_STR_PREFIX(type, tdef);
		CASE_STR_PREFIX(type, ptr);
		CASE_STR_PREFIX(type, block);
		CASE_STR_PREFIX(type, func);
		CASE_STR_PREFIX(type, array);
		CASE_STR_PREFIX(type, cast);
		CASE_STR_PREFIX(type, attr);
		CASE_STR_PREFIX(type, where);
	}
	ucc_unreach(NULL);
}

enum type_str_type
type_str_type(type *r)
{
	type *t = type_is_array(r);
	if(!t)
		t = type_is_ptr(r);
	t = type_is_primitive(t, type_unknown);
	switch(t ? t->bits.type->primitive : type_unknown){
		case type_schar:
		case type_nchar:
		case type_uchar:
			return type_str_char;

		case type_int:
			return type_str_wchar;

		default:
			return type_str_no;
	}
}
