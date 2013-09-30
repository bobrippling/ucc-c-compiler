#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/dynmap.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "cc1.h"
#include "fold.h"
#include "const.h"
#include "macros.h"
#include "sue.h"
#include "ops/__builtin.h"

#include "decl_init.h"

#ifdef DEBUG_DECL_INIT
static int init_debug_depth;

ucc_printflike(1, 2) void INIT_DEBUG(const char *fmt, ...)
{
	va_list l;
	int i;

	for(i = init_debug_depth; i > 0; i--)
		fputs("  ", stderr);

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
}

#  define INIT_DEBUG_DEPTH(op) init_debug_depth op
#else
#  define INIT_DEBUG_DEPTH(op)
#  define INIT_DEBUG(...)
#endif

typedef struct
{
	decl_init **pos;
} init_iter;

#define ITER_WHERE(it, def) \
	(it && it->pos && it->pos[0] && it->pos[0] != DYNARRAY_NULL \
	 ? &it->pos[0]->where \
	 : def)

#define DECL_IS_ANON_BITFIELD(d) ((d)->field_width && !(d)->spel)

typedef decl_init **aggregate_brace_f(
		decl_init **current, struct init_cpy ***range_store,
		init_iter *,
		symtable *,
		void *, int);

static decl_init *decl_init_brace_up_aggregate(
		decl_init *current,
		init_iter *iter,
		symtable *stab,
		type_ref *tfor,
		aggregate_brace_f *,
		void *arg1, int arg2);

/* null init are const/zero, flag-init is const/zero if prev. is const/zero,
 * which will be checked elsewhere */
#define DINIT_NULL_CHECK(di) \
	if(di == DYNARRAY_NULL)    \
		return 1

static struct init_cpy *init_cpy_from_dinit(decl_init *di)
{
	struct init_cpy *cpy = umalloc(sizeof *cpy);
	cpy->range_init = di;
	return cpy;
}

int decl_init_is_const(decl_init *dinit, symtable *stab)
{
	DINIT_NULL_CHECK(dinit);

	switch(dinit->type){
		case decl_init_scalar:
		{
			expr *e;
			consty k;

			e = FOLD_EXPR(dinit->bits.expr, stab);
			const_fold(e, &k);

			return CONST_AT_COMPILE_TIME(k.type);
		}

		case decl_init_brace:
		{
			decl_init **i;

			for(i = dinit->bits.ar.inits; i && *i; i++)
				if(!decl_init_is_const(*i, stab))
					return 0;

			return 1;
		}

		case decl_init_copy:
			return 1;
	}

	ICE("bad decl init");
	return -1;
}

int decl_init_is_zero(decl_init *dinit)
{
	DINIT_NULL_CHECK(dinit);

	switch(dinit->type){
		case decl_init_scalar:
			return const_expr_and_zero(dinit->bits.expr);

		case decl_init_brace:
		{
			decl_init **i;

			for(i = dinit->bits.ar.inits; i && *i; i++)
				if(!decl_init_is_zero(*i))
					return 0;

			return 1;
		}

		case decl_init_copy:
		{
			struct init_cpy *cpy = *dinit->bits.range_copy;
			return decl_init_is_zero(cpy->range_init);
		}
	}

	ICE("bad decl init type %d", dinit->type);
	return -1;
}

decl_init *decl_init_new_w(enum decl_init_type t, where *w)
{
	decl_init *di = umalloc(sizeof *di);
	if(w)
		memcpy_safe(&di->where, w);
	else
		where_new(&di->where);
	di->type = t;
	return di;
}

decl_init *decl_init_new(enum decl_init_type t)
{
	return decl_init_new_w(t, NULL);
}

static decl_init *decl_init_copy_const(decl_init *di)
{
	decl_init *ret;

	if(di == DYNARRAY_NULL)
		return di;

	ret = umalloc(sizeof *ret);
	memcpy_safe(ret, di);

	switch(ret->type){
		case decl_init_scalar:
		case decl_init_copy:
			/* no need to copy these - treated as immutable */
			break;
		case decl_init_brace:
		{
			/* need to copy inits as we may replace copy-ees'
			 * sub inits via designations */
			decl_init **inits = di->bits.ar.inits;
			size_t i;

			ret->bits.ar.inits = umalloc((dynarray_count(inits) + 1) * sizeof *inits);
			for(i = 0; inits[i]; i++)
				ret->bits.ar.inits[i] = decl_init_copy_const(di->bits.ar.inits[i]);
			break;
		}
	}

	return ret;
}

static void decl_init_resolve_copy(decl_init **arr, const size_t idx)
{
	decl_init *resolved = arr[idx];
	struct init_cpy *cpy;

	UCC_ASSERT(resolved->type == decl_init_copy,
			"resolving a non-copy (%d)", resolved->type);

	cpy = *resolved->bits.range_copy;
	memcpy_safe(resolved, decl_init_copy_const(cpy->range_init));
}

static void decl_init_free_1(decl_init *di)
{
	free(di);
}

const char *decl_init_to_str(enum decl_init_type t)
{
	switch(t){
		CASE_STR_PREFIX(decl_init, scalar);
		CASE_STR_PREFIX(decl_init, brace);
		CASE_STR_PREFIX(decl_init, copy);
	}
	return NULL;
}

/*
 * brace up code
 * -------------
 */

static decl_init *decl_init_brace_up_r(decl_init *current, init_iter *, type_ref *, symtable *stab);

static void override_warn(
		type_ref *tfor, where *old, where *new, int whole)
{
	char buf[WHERE_BUF_SIZ];

	warn_at(new,
			"overriding %sinitialisation of \"%s\"\n"
			"%s: prior initialisation here",
			whole ? "entire " : "",
			type_ref_to_str(tfor),
			where_str_r(buf, old));
}

static void excess_init(where *w, type_ref *ty)
{
	warn_at(w, "excess initialiser for '%s'", type_ref_to_str(ty));
}

static decl_init *decl_init_brace_up_scalar(
		decl_init *current, init_iter *iter, type_ref *const tfor,
		symtable *stab)
{
	decl_init *first_init;
	where *const w = ITER_WHERE(iter, &tfor->where);

	if(current){
		override_warn(tfor, &current->where, w, 0);

		decl_init_free_1(current);
	}

	if(!iter->pos || !*iter->pos){
		first_init = decl_init_new_w(decl_init_scalar, w);
		first_init->bits.expr = expr_new_val(0); /* default init for everything */
		return first_init;
	}

	first_init = *iter->pos++;

	if(first_init->desig)
		die_at(&first_init->where, "initialising scalar with %s designator",
				DESIG_TO_STR(first_init->desig->type));

	if(first_init->type == decl_init_brace){
		init_iter it;
		unsigned n;

		it.pos = first_init->bits.ar.inits;

		n = dynarray_count(it.pos);
		if(n > 1)
			excess_init(&first_init->where, tfor);

		return decl_init_brace_up_r(current, &it, tfor, stab);
	}

	/* fold */
	FOLD_EXPR(first_init->bits.expr, stab);

	fold_type_chk_and_cast(
			tfor, &first_init->bits.expr,
			stab, &first_init->bits.expr->where,
			"initialisation");

	return first_init;
}

static void range_store_add(
		struct init_cpy ***range_store,
		struct init_cpy *entry,
		decl_init **updataable_refs)
{
	long *offsets = NULL, *off;
	decl_init **i;

	for(i = updataable_refs; i && *i; i++){
		decl_init *ent = *i;
		if(ent != DYNARRAY_NULL && ent->type == decl_init_copy){
			/* this entry's copy points into range_store,
			 * and will need updating */
			dynarray_add(&offsets,
					(long)(1 + DECL_INIT_COPY_IDX_INITS(ent, *range_store)));

			/* +1 because dynarray doesn't allow NULL */
		}
	}

	dynarray_add(range_store, entry);

	off = offsets;
	for(i = updataable_refs; i && *i; i++){
		decl_init *ent = *i;
		if(ent != DYNARRAY_NULL && ent->type == decl_init_copy)
			/* -1 explained above */
			ent->bits.range_copy = *range_store + (*off++ - 1);
	}

	free(offsets);
}

static decl_init **decl_init_brace_up_array2(
		decl_init **current, struct init_cpy ***range_store,
		init_iter *iter,
		symtable *stab,
		type_ref *next_type, const int limit)
{
	unsigned n = dynarray_count(current), i = 0, j = 0;
	decl_init *this;

	while((this = *iter->pos)){
		desig *des;

		if((des = this->desig)){
			consty k[2];

			this->desig = des->next;

			if(des->type != desig_ar){
				die_at(&this->where,
						"%s designator can't designate array",
						DESIG_TO_STR(des->type));
			}

			FOLD_EXPR(des->bits.range[0], stab);
			const_fold(des->bits.range[0], &k[0]);

			if(des->bits.range[1]){
				FOLD_EXPR(des->bits.range[1], stab);
				const_fold(des->bits.range[1], &k[1]);
			}else{
				memcpy(&k[1], &k[0], sizeof k[1]);
			}

			if(k[0].type != CONST_NUM || k[1].type != CONST_NUM)
				die_at(&this->where, "non-constant array-designator");
			if((k[0].bits.num.suffix | k[1].bits.num.suffix) & VAL_FLOATING)
				die_at(&this->where, "non-integral array-designator");

			if((sintegral_t)k[0].bits.num.val.i < 0 || (sintegral_t)k[1].bits.num.val.i < 0)
				die_at(&this->where, "negative array index initialiser");

			if(limit > -1
			&& (k[0].bits.num.val.i >= (integral_t)limit
			||  k[1].bits.num.val.i >= (integral_t)limit))
			{
				die_at(&this->where, "designating outside of array bounds (%d)", limit);
			}

			i = k[0].bits.num.val.i;
			j = k[1].bits.num.val.i;
		}else if(limit > -1 && i >= (unsigned)limit){
			break;
		}

		{
			decl_init *replacing = NULL, *replace_save = NULL;
			unsigned replace_idx;
			decl_init *braced;
			int partial_replace = 0;

			if(i < n && current[i] != DYNARRAY_NULL){
				replacing = current[i]; /* replacing object `i' */

				/* we can't designate sub parts of a [x ... y] subobject yet,
				 * as this requires being able to copy the init from x to y,
				 * then replace a subobject in y, which we don't have the data
				 * structures to do
				 *
				 * disallow, unless it's of scalar type
				 * i.e. x[] = { [0 ... 2] = f(), [1] = 5 }
				 * ^ fine, as we can't replace subobjects of scalars
				 */
				if(replacing != DYNARRAY_NULL
				&& this->type != decl_init_brace
				&& !type_ref_is_scalar(next_type)){
					/* we can replace brace inits IF they're constant (as a special
					 * case), which is usually a common usage for static/global inits
					 */
					if(!decl_init_is_const(replacing, stab)){
						char wbuf[WHERE_BUF_SIZ];

						die_at(&this->where,
								"can't replace _part_ of array-range subobject without braces\n"
								"%s: array range here", where_str_r(wbuf, &replacing->where));
					}

					/*
					 * else - partial replacement
					 * resolve the copy, then pass the copy down so sub-inits can
					 * update/replace it
					 */
					partial_replace = 1;

					if(replacing->type == decl_init_copy){
						decl_init_resolve_copy(current, i);
						replacing = current[i];
					}
				}

				if(!partial_replace)
					replacing = NULL; /* prevent free + we're starting anew */
			}

			if(partial_replace && i < j){
				/* we're replacing something with a range */
				replace_save = decl_init_copy_const(replacing);
			}

			/* check for char[] init */
			braced = decl_init_brace_up_r(replacing, iter, next_type, stab);

			dynarray_padinsert(&current, i, &n, braced);

			if(i < j){ /* then we have a range to copy */
				const size_t copy_idx = dynarray_count(*range_store);

				/* if we've replaced something existing with a range, e.g.
				 * typedef struct { int i, j; } pair;
				 * pair x[] = { { 1, 2 }, [0 ... 5].j = 3 };
				 *
				 * we want: 1, 3, { 0, 3 }...
				 *
				 * so we keep the { 1, 3 } custom like it is,
				 * and add replace_save to the range_stores instead
				 */

				/* keep track of the initial copy ({ 1, 3 })
				 * aggregate in .range_store */
				range_store_add(range_store, init_cpy_from_dinit(braced), current);

				if(replace_save){
					/* keep track of what we replaced */
					range_store_add(range_store,
							init_cpy_from_dinit(replace_save), current);
				}

				for(replace_idx = i; replace_idx <= j; replace_idx++){
					decl_init *cpy = decl_init_new_w(decl_init_copy, &braced->where);

					if(partial_replace){
						decl_init *old = replace_idx < n ? current[replace_idx] : NULL;

						if(old == DYNARRAY_NULL)
							old = NULL;

						if(old){
							die_at(&old->where, "can't replace with a range currently");
						}
					}

					cpy->bits.range_copy = &(*range_store)[copy_idx];

					dynarray_padinsert(&current, replace_idx, &n, cpy);
				}
			}
		}

		i++;
		j++;
	}

	return current;
}


static decl_init **decl_init_brace_up_sue2(
		decl_init **current, decl_init ***range_store,
		init_iter *iter,
		symtable *stab,
		struct_union_enum_st *sue, const int is_anon)
{
	unsigned n = dynarray_count(current), i;
	unsigned sue_nmem;
	int had_desig = 0;
	where *last_loc = NULL;
	decl_init *this;

	(void)range_store;

	UCC_ASSERT(sue_complete(sue), "should've checked sue completeness");

	/* check for copy-init */
	if((this = *iter->pos) && this->type == decl_init_scalar){
		expr *e = FOLD_EXPR(this->bits.expr, stab);

		if(type_ref_is_s_or_u(e->tree_type) == sue){
			/* copy init */
			dynarray_padinsert(&current, 0, &n, this);

			++iter->pos;

			return current;
		}
	}

	sue_nmem = sue_nmembers(sue);
	/* check for {} */
	if(sue_nmem == 0
	&& (this = *iter->pos)
	&& (this->type != decl_init_brace
		|| dynarray_count(this->bits.ar.inits) != 0))
	{
		warn_at(&this->where, "missing {} initialiser for empty %s",
				sue_str(sue), sue->spel);
	}

	for(i = 0; (this = *iter->pos); i++){
		desig *des;
		decl_init *braced_sub = NULL;

		last_loc = &this->where;

		if((des = this->desig)){
			/* find member, set `i' to its index */
			struct_union_enum_st *in;
			decl *mem;
			unsigned j = 0;
			int found = 0;

			if(des->type != desig_struct){
				die_at(&this->where,
						"%s designator can't designate struct",
						DESIG_TO_STR(des->type));
			}

			had_desig = 1;

			this->desig = des->next;

			mem = struct_union_member_find(sue, des->bits.member, &j, &in);
			if(!mem){
				/* if we're an anonymous struct, return out */
				if(is_anon){
					this->desig = des;
					break;
				}

				die_at(&this->where,
						"%s %s contains no such member \"%s\"",
						sue_str(sue), sue->spel, des->bits.member);
			}

			for(j = 0; sue->members[j]; j++){
				decl *jmem = sue->members[j]->struct_member;

				if(jmem == mem){
					found = 1;
				}else if(!jmem->spel && in){
					struct_union_enum_st *jmem_sue = type_ref_is_s_or_u(jmem->ref);
					if(jmem_sue == in){
						decl_init *replacing;

						/* anon struct/union, sub init it, restoring the desig. */
						this->desig = des;

						replacing = j < n
							&& current[j] != DYNARRAY_NULL ? current[j] : NULL;

						braced_sub = decl_init_brace_up_aggregate(
								replacing, iter, stab, jmem->ref,
								(aggregate_brace_f *)&decl_init_brace_up_sue2, in, 1);

						found = 1;
					}
				}
				if(found){
					i = j;
					break;
				}
			}

			if(!found)
				ICE("couldn't find member %s", des->bits.member);
		}

		if(i < sue_nmem){
			sue_member *mem = sue->members[i];
			decl_init *replacing = NULL;
			decl *d_mem;

			if(!mem)
				break;
			d_mem = mem->struct_member;

			/* skip bitfield padding
			 * init for it is <zero> created by a dynarray_padinsert */
			if(DECL_IS_ANON_BITFIELD(d_mem))
				continue;

			if(i < n && current[i] != DYNARRAY_NULL)
				replacing = current[i];

			if(type_ref_is_incomplete_array(d_mem->ref)){
				warn_at(&this->where, "initialisation of flexible array (GNU)");
			}

			if(!braced_sub){
				braced_sub = decl_init_brace_up_r(
						replacing, iter,
						d_mem->ref, stab);
			}

			/* XXX: padinsert will insert zero inits for skipped fields,
			 * including anonymous bitfield pads
			 */
			dynarray_padinsert(&current, i, &n, braced_sub);

			/* done, check bitfield truncation */
			if(braced_sub && d_mem->field_width){
				UCC_ASSERT(braced_sub->type == decl_init_scalar,
						"scalar init expected for bitfield");
				bitfield_trunc_check(d_mem, braced_sub->bits.expr);
			}

			if(sue->primitive == type_union)
				break;
		}else{
			break;
		}
	}

	if(sue->primitive == type_struct
	&& !had_desig /* don't warn for designated inits */
	&& i < sue_nmem)
	{
		const unsigned diff = sue_nmem - i;
		where *loc = ITER_WHERE(iter, last_loc ? last_loc : &sue->where);

		warn_at(loc,
				"%u missing initialiser%s for '%s %s'",
				diff, diff == 1 ? "" : "s",
				sue_str(sue), sue->spel);
	}

	return current;
}

static int find_desig(decl_init **const ar)
{
	decl_init **i, *d;

	for(i = ar; (d = *i); i++)
		if(d->desig)
			return i - ar;

	return -1;
}

static decl_init *decl_init_brace_up_aggregate(
		decl_init *current,
		init_iter *iter,
		symtable *stab, type_ref *tfor,
		aggregate_brace_f *brace_up_f,
		void *arg1, int arg2)
{
	/* we don't pass through iter in the case that:
	 * we are brace or next is a designator, i.e.
	 *
	 * struct A
	 * {
	 *   struct
	 *   {
	 *     int sub1, sub2, sub3, ...;
	 *   } sub;
	 *   int i;
	 * };
	 *
	 * struct A x = {
	 *   { 1 }, 2 // we've specified sub with a brace
	 *            // don't pass through `2'
	 * };
	 *
	 * struct A y = {
	 *    1, 3, .i = 2 // initialise sub1 with { 1, 3 },
	 *              // but don't pass .i=2 to the sub-init
	 * };
	 */
	int desig_index;

	if(iter->pos[0]->type == decl_init_brace){
		/* pass down this as a new iterator */
		decl_init *first = iter->pos[0];
		decl_init **old_subs = first->bits.ar.inits;

		if(old_subs){
			init_iter it;

			it.pos = old_subs;

			/* prevent designator loss */
			if(first->desig){
				/* need to insert our desig */
				struct desig *sub_d = old_subs[0]->desig, *di;

				/* insert */
				old_subs[0]->desig = first->desig;

				/* tack old on the end */
				for(di = old_subs[0]->desig; di->next; di = di->next);
				di->next = sub_d;

			}else if(current){ /* gcc (not clang) compliant */
				/* we have no sub-designator - we're overriding an entire sub object */

				override_warn(tfor, &current->where, &first->where, 1 /* whole */);
				/* XXX: memleak decl_init_free(current); */
				current = NULL; /* prevent any current init getting through */
			}

			first->bits.ar.inits = brace_up_f(
					current ? current->bits.ar.inits : NULL,
					/* clang would always pass current->bits.inits through here
					 * and not current=NULL above
					 */
					&first->bits.ar.range_inits,
					&it,
					stab, arg1, arg2);

			if(it.pos[0]){
				/* we know we're in a brace,
				 * so it.pos... etc aren't for anything else */
				excess_init(&it.pos[0]->where, tfor);
			}

			free(old_subs);

		}else{
			/* {} */
			first->bits.ar.inits = NULL;
			dynarray_add(&first->bits.ar.inits, (decl_init *)DYNARRAY_NULL);
		}

		++iter->pos;
		return first; /* this is in the {} state */

	}else if((desig_index = find_desig(iter->pos + 1)) >= 0){
		decl_init *const saved = iter->pos[++desig_index];
		decl_init *ret = decl_init_new_w(decl_init_brace, ITER_WHERE(iter, NULL));
		init_iter it = { iter->pos };

		iter->pos[desig_index] = NULL;

		ret->bits.ar.inits = brace_up_f(
				current ? current->bits.ar.inits : NULL,
				&ret->bits.ar.range_inits,
				&it, stab, arg1, arg2);

		iter->pos[desig_index] = saved;

		/* need to increment only by the amount that was used */
		iter->pos += it.pos - iter->pos;

		return ret;
	}else{
		where *loc = ITER_WHERE(iter, NULL);
		decl_init *r = decl_init_new_w(decl_init_brace, loc);

		warn_at(loc, "missing braces for initialisation of sub-object '%s'",
				type_ref_to_str(tfor));

		/* we need to pull from iter, bracing up our children inits */
		r->bits.ar.inits = brace_up_f(
				current ? current->bits.ar.inits : NULL,
				&r->bits.ar.range_inits,
				iter, stab, arg1, arg2);

		return r;
	}
}

static void die_incomplete(init_iter *iter, type_ref *tfor)
{
	struct_union_enum_st *sue = type_ref_is_s_or_u(tfor);
	if(sue)
		sue_incomplete_chk(sue, ITER_WHERE(iter, &sue->where));

	die_at(ITER_WHERE(iter, &tfor->where),
			"initialising %s", type_ref_to_str(tfor));
}

static decl_init *decl_init_brace_up_array_pre(
		decl_init *current, init_iter *iter,
		type_ref *next_type, symtable *stab)
{
	const int limit = type_ref_is_incomplete_array(next_type)
		? -1 : (signed)type_ref_array_len(next_type);

	type_ref *next = type_ref_next(next_type);

	const int for_str_ar = !!type_ref_is_type(
			type_ref_is_array(next_type), type_nchar);

	decl_init *this;

	if(!type_ref_is_complete(next))
		die_incomplete(iter, next_type);

	if(for_str_ar
	&& (this = *iter->pos)
	/* allow "xyz" or { "xyz" } */
	&& (this->type == decl_init_scalar
	|| (this->type == decl_init_brace &&
		  1 == dynarray_count(this->bits.ar.inits) &&
		  this->bits.ar.inits[0]->type == decl_init_scalar)))
	{
		decl_init *strk = this->type == decl_init_scalar
			? this : this->bits.ar.inits[0];

		consty k;

		FOLD_EXPR(strk->bits.expr, stab);
		const_fold(strk->bits.expr, &k);

		if(k.type == CONST_STRK){
			where *const w = &strk->where;
			unsigned str_i;

			if(k.bits.str->wide)
				ICE("TODO: wide string init");

			decl_init *braced = decl_init_new_w(decl_init_brace, w);

			for(str_i = 0; str_i < k.bits.str->len; str_i++){
				decl_init *char_init = decl_init_new_w(decl_init_scalar, w);

				char_init->bits.expr = expr_new_val(k.bits.str->str[str_i]);

				dynarray_add(&braced->bits.ar.inits, char_init);
			}

			++iter->pos;

			return braced;
		}
	}

	return decl_init_brace_up_aggregate(
			current, iter, stab, next_type,
			(aggregate_brace_f *)&decl_init_brace_up_array2,
			type_ref_next(next_type), limit);
}


static decl_init *decl_init_brace_up_r(
		decl_init *current, init_iter *iter,
		type_ref *tfor, symtable *stab)
{
	struct_union_enum_st *sue;

	fold_type_ref(tfor, NULL, stab);

	if(type_ref_is(tfor, type_ref_array))
		return decl_init_brace_up_array_pre(
				current, iter, tfor, stab);

	/* incomplete check _after_ array, since we allow T x[] */
	if(!type_ref_is_complete(tfor))
		die_incomplete(iter, tfor);

	if((sue = type_ref_is_s_or_u(tfor)))
		return decl_init_brace_up_aggregate(
				current, iter, stab, tfor,
				(aggregate_brace_f *)&decl_init_brace_up_sue2,
				sue, 0 /* is anon */);

	return decl_init_brace_up_scalar(current, iter, tfor, stab);
}

static decl_init *decl_init_brace_up_start(
		decl_init *init, type_ref **ptfor,
		symtable *stab)
{
	decl_init *inits[2] = {
		init, NULL
	};
	init_iter it = { inits };
	type_ref *const tfor = *ptfor;
	decl_init *ret;

	fold_type_ref(tfor, NULL, stab);

	/* check for non-brace init */
	if(init
	&& init->type == decl_init_scalar
	&& type_ref_is_array(tfor))
	{
		expr *e = FOLD_EXPR(init->bits.expr, stab);

		if(type_ref_cmp(e->tree_type, tfor, 0) != TYPE_EQUAL){
			die_at(&init->where,
					"%s must be initialised with an initialiser list",
					type_ref_to_str(tfor));
		}
		/* else struct copy init */
	}

	ret = decl_init_brace_up_r(NULL, &it, tfor, stab);

	if(type_ref_is_incomplete_array(tfor)){
		/* complete it */
		UCC_ASSERT(ret->type == decl_init_brace, "unbraced array");
		*ptfor = type_ref_complete_array(tfor, dynarray_count(ret->bits.ar.inits));
	}

	return ret;
}

void decl_init_brace_up_fold(decl *d, symtable *stab)
{
	if(!d->init_normalised){
		d->init = decl_init_brace_up_start(d->init, &d->ref, stab);
		d->init_normalised = 1;
	}
}


static expr *decl_init_create_assignments_sue_base(
		struct_union_enum_st *sue, expr *base,
		decl **psmem,
		unsigned idx, unsigned n)
{
	decl *smem;

	UCC_ASSERT(idx < n, "oob member init");

	*psmem = smem = sue->members[idx]->struct_member;

	/* don't create zero-width bitfield inits */
	if(DECL_IS_ANON_BITFIELD(smem)
	&& const_expr_and_zero(smem->field_width))
	{
		return NULL;
	}

	return expr_new_struct_mem(base, 1, smem);
}

static void decl_init_create_assignment_from_copy(
		decl_init *di, stmt *code,
		type_ref *next_type, expr *new_base)
{
	/* TODO: ideally when the backend is sufficiently optimised
	 * it'll pick it up the memcpy well
	 */
	struct init_cpy *icpy = *di->bits.range_copy;

	UCC_ASSERT(next_type, "no next type for array");

	/* memcpy from the previous init */
	if(icpy->first_instance){
		expr *last_base = icpy->first_instance;

		expr *memcp = builtin_new_memcpy(
				new_base, last_base, type_ref_size(next_type, &di->where));

		dynarray_add(&code->codes,
				expr_to_stmt(memcp, code->symtab));
	}else{
		/* the initial assignment from the range_copy */
		icpy->first_instance = new_base;

		decl_init_create_assignments_base(icpy->range_init,
				next_type, new_base, code);
	}
}

void decl_init_create_assignments_base(
		decl_init *init,
		type_ref *tfor, expr *base,
		stmt *code)
{
	if(!init){
		expr *zero;

zero_init:
		if(type_ref_is_incomplete_array(tfor)){
			/* error caught elsewhere,
			 * where we can print the location */
			return;
		}

		/* this works for zeroing bitfields,
		 * since we don't take the address
		 * - builtin memset calls lea_expr()
		 *   which can handle bitfields
		 */
		zero = builtin_new_memset(
				base,
				0,
				type_ref_size(tfor, &base->where));

		dynarray_add(
				&code->codes,
				expr_to_stmt(zero, code->symtab));
		return;
	}

	switch(init->type){
		case decl_init_scalar:
			dynarray_add(
					&code->codes,
					expr_to_stmt(
						expr_new_assign_init(base, init->bits.expr),
						code->symtab));
			break;

		case decl_init_copy:
			ICE("copy got through assignment");

		case decl_init_brace:
		{
			struct_union_enum_st *sue = type_ref_is_s_or_u(tfor);
			size_t n;
			decl_init **i;
			unsigned idx;

			if(sue /* check for struct copy */
			&& dynarray_count(init->bits.ar.inits) == 1
			&& init->bits.ar.inits[0] != DYNARRAY_NULL
			&& init->bits.ar.inits[0]->type == decl_init_scalar)
			{
				expr *e = init->bits.ar.inits[0]->bits.expr;

				if(type_ref_is_s_or_u(e->tree_type) == sue){
					dynarray_add(
							&code->codes,
							expr_to_stmt(
								builtin_new_memcpy(
									base, e, type_ref_size(e->tree_type, &e->where)),
								code->symtab));
					return;
				}
			}

			/* type_ref_array_len()
			 * we're already braced so there are no incomplete arrays
			 * except for C99 flexible arrays
			 */
			if(sue){
				n = dynarray_count(sue->members);
			}else if(type_ref_is_incomplete_array(tfor)){
				n = dynarray_count(init->bits.ar.inits);

				/* it's fine if there's nothing for it */
				if(n > 0)
					die_at(&init->where, "non-static initialisation of flexible array");
			}else{
				n = type_ref_array_len(tfor);
			}

			/* check union */
			if(sue && sue->primitive == type_union){
				decl *smem;
				expr *sue_base;

				/* look for a non null init */
				for(idx = 0, i = init->bits.ar.inits; *i == DYNARRAY_NULL; i++, idx++);

				if(*i){
					sue_base = decl_init_create_assignments_sue_base(
							sue, base, &smem, idx, n);

					UCC_ASSERT(sue_base, "zero width bitfield init in union?");

					decl_init_create_assignments_base(
							*i,
							smem->ref,
							sue_base,
							code);
				}else{
					/* zero init union - make sure we get all of it */
					goto zero_init;
				}
				return;
			}

			for(idx = 0, i = init->bits.ar.inits; idx < n; (*i ? i++ : 0), idx++){
				decl_init *di = *i;
				expr *new_base;
				type_ref *next_type = NULL;

				if(di == DYNARRAY_NULL)
					di = NULL;

				if(sue){
					decl *smem = sue->members[idx]->struct_member;

					UCC_ASSERT(sue->primitive != type_union, "sneaky union");

					new_base = decl_init_create_assignments_sue_base(
							sue, base, &smem, idx, n);

					if(!new_base)
						continue; /* 0-width bitfield */

					next_type = smem->ref;

				}else{
					new_base = expr_new_array_idx(base, idx);

					if(!next_type)
						next_type = type_ref_next(tfor);

					if(di && di != DYNARRAY_NULL && di->type == decl_init_copy){
						decl_init_create_assignment_from_copy(
								di, code, next_type, new_base);
						continue;
					}
				}

				decl_init_create_assignments_base(di, next_type, new_base, code);
			}
			break;
		}
	}
}

void decl_default_init(decl *d, symtable *stab)
{
	UCC_ASSERT(!d->init, "already initialised?");

	d->init = decl_init_new_w(decl_init_brace, &d->where);
	decl_init_brace_up_fold(d, stab);
}
