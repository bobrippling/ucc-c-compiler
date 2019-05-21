#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/dynmap.h"
#include "../util/alloc.h"
#include "cc1.h"
#include "fold.h"
#include "const.h"
#include "macros.h"
#include "sue.h"
#include "ops/__builtin.h"
#include "cc1_where.h"
#include "decl_init.h"
#include "type_is.h"
#include "ops/expr_compound_lit.h"
#include "ops/expr_string.h"
#include "ops/expr_val.h"
#include "ops/expr_assign.h"
#include "ops/expr_struct.h"
#include "ops/expr_comma.h"
#include "str.h"

#include "fopt.h"

typedef struct
{
	decl_init **pos;
} init_iter;

#define ITER_WHERE(it, def) \
	(it && it->pos && it->pos[0] && it->pos[0] != DYNARRAY_NULL \
	 ? &it->pos[0]->where \
	 : def)

typedef decl_init **aggregate_brace_f(
		decl_init **current, struct init_cpy ***range_store,
		init_iter *,
		symtable *,
		void *arg1, int arg2, int allow_struct_copy);

static decl_init *decl_init_brace_up_aggregate(
		decl_init *current,
		init_iter *iter,
		symtable *stab,
		type *tfor,
		aggregate_brace_f *,
		void *arg1, int arg2);

static void decl_init_create_assignments_base(
		decl_init *init,
		type *tfor, expr *base,
		expr **pinit,
		symtable *stab,
		int aggregate);

/* null init are const/zero, flag-init is const/zero if prev. is const/zero,
 * which will be checked elsewhere */
#define DINIT_NULL_CHECK(di, fallback) \
	if(di == DYNARRAY_NULL)    \
		fallback

#define init_debug_indent(op) init_indent op

static int init_indent;

static void init_indent_out(void)
{
	int i;
	for(i = 0; i < init_indent; i++)
		fputs("  ", stderr);
}

ucc_printflike(1, 2)
static void init_debug(const char *fmt, ...)
{
	va_list l;

	if(!cc1_fopt.dump_init)
		return;

	init_indent_out();
	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
}

ucc_printflike(1, 2)
static void init_debug_noindent(const char *fmt, ...)
{
	va_list l;

	if(!cc1_fopt.dump_init)
		return;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);
}

static void init_debug_dinit(init_iter *init_iter, type *tfor)
{
	where dummy_where = { 0 };

	if(!cc1_fopt.dump_init)
		return;

	dummy_where.fname = "<n/a>";

	init_debug_noindent("%s --> %s [%s]\n",
			init_iter && init_iter->pos
			? decl_init_to_str(init_iter->pos[0]->type)
			: "[nil]",
			type_to_str(tfor),
			where_str(ITER_WHERE(init_iter, &dummy_where)));
}

static void init_debug_desig(struct desig *desig, symtable *stab)
{
	if(!cc1_fopt.dump_init)
		return;

	if(!desig)
		init_debug_noindent("<empty>");

	for(; desig; desig = desig->next){
		switch(desig->type){
			case desig_ar:
				FOLD_EXPR(desig->bits.range[0], stab);

				if(desig->bits.range[1]){
					FOLD_EXPR(desig->bits.range[1], stab);

					init_debug_noindent("[%" NUMERIC_FMT_U " ...  %" NUMERIC_FMT_U "]",
							const_fold_val_i(desig->bits.range[0]),
							const_fold_val_i(desig->bits.range[1]));
				}else{
					init_debug_noindent("[%" NUMERIC_FMT_U "]",
							const_fold_val_i(desig->bits.range[0]));
				}
				break;
			case desig_struct:
				init_debug_noindent(".%s", desig->bits.member);
				break;
		}
	}

	fputc('\n', stderr);
}

static struct init_cpy *init_cpy_from_dinit(decl_init *di)
{
	struct init_cpy *cpy = umalloc(sizeof *cpy);
	cpy->range_init = di;
	return cpy;
}

int decl_init_is_const(
		decl_init *dinit, symtable *stab,
		type *expected_ty, expr **const nonstd, expr **const nonconst)
{
	DINIT_NULL_CHECK(dinit, return 1);

	assert(expected_ty);

	switch(dinit->type){
		case decl_init_scalar:
		{
			expr *e;
			consty k;
			int ret;

			e = FOLD_EXPR(dinit->bits.expr, stab);
			const_fold(e, &k);

			if(k.nonstandard_const && nonstd && !*nonstd)
				*nonstd = k.nonstandard_const;

			ret = CONST_AT_COMPILE_TIME(k.type);

			if(!ret && nonconst && !*nonconst)
				*nonconst = k.nonconst;

			return ret;
		}

		case decl_init_brace:
		{
			size_t i;
			type *sub;
			expr *copy_from;
			struct_union_enum_st *su = type_is_s_or_u(expected_ty);

			if(su && (copy_from = decl_init_is_struct_copy(dinit, su))){
				copy_from = expr_skip_lval2rval(copy_from);

				if(expr_kind(copy_from, compound_lit)
				&& type_is_s_or_u(copy_from->tree_type) == type_is_s_or_u(expected_ty))
				{
					*nonstd = copy_from;
					return 1;
				}
			}

			if(!dinit->bits.ar.inits){
				/* empty */
				return 1;
			}

			sub = type_is_array(expected_ty);

			for(i = 0; ; i++){
				decl_init *init = dinit->bits.ar.inits[i];
				type *current_ty = sub;

				if(!init){
					break;
				}

				if(!current_ty){
					/* struct/union */
					assert(su);
					if(su->members){
						current_ty = su->members[i]->struct_member->ref;
						assert(current_ty);
					}else{
						current_ty = NULL;
						assert(init == DYNARRAY_NULL);
					}
				}

				if(!decl_init_is_const(init, stab, current_ty, nonstd, nonconst))
					return 0;
			}

			return 1;
		}

		case decl_init_copy:
		{
			struct init_cpy *cpy = *dinit->bits.range_copy;
			return decl_init_is_const(cpy->range_init, stab, expected_ty, nonstd, nonconst);
		}
	}

	ICE("bad decl init");
	return -1;
}

static int decl_init_is_zero_fold(decl_init *dinit, symtable *symtab)
{
	DINIT_NULL_CHECK(dinit, return 1);

	switch(dinit->type){
		case decl_init_scalar:
			if(symtab)
				fold_expr_nodecay(dinit->bits.expr, symtab);
			return const_expr_and_zero(dinit->bits.expr);

		case decl_init_brace:
		{
			decl_init **i;

			for(i = dinit->bits.ar.inits; i && *i; i++)
				if(!decl_init_is_zero_fold(*i, symtab))
					return 0;

			return 1;
		}

		case decl_init_copy:
		{
			struct init_cpy *cpy = *dinit->bits.range_copy;
			return decl_init_is_zero_fold(cpy->range_init, symtab);
		}
	}

	ICE("bad decl init type %d", dinit->type);
	return -1;
}

int decl_init_is_zero(decl_init *dinit)
{
	return decl_init_is_zero_fold(dinit, NULL);
}

int decl_init_has_sideeffects(decl_init *dinit)
{
	DINIT_NULL_CHECK(dinit, return 0);

	switch(dinit->type){
		case decl_init_scalar:
			return expr_has_sideeffects(dinit->bits.expr);

		case decl_init_brace:
		{
			decl_init **i;

			for(i = dinit->bits.ar.inits; i && *i; i++)
				if(decl_init_has_sideeffects(*i))
					return 1;

			return 0;
		}

		case decl_init_copy:
		{
			struct init_cpy *cpy = *dinit->bits.range_copy;
			return decl_init_has_sideeffects(cpy->range_init);
		}
	}

	return 0;
}

decl_init *decl_init_new_w(enum decl_init_type t, where *w)
{
	decl_init *di = umalloc(sizeof *di);
	if(w)
		memcpy_safe(&di->where, w);
	else
		where_cc1_current(&di->where);
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

static decl_init *decl_init_brace_up_r(decl_init *current, init_iter *,
		type *, symtable *stab);

static void override_warn(
		type *tfor, where *old, where *new, int whole)
{
	if(cc1_warn_at(new,
			init_override,
			"overriding %sinitialisation of \"%s\"",
			whole ? "entire " : "",
			type_to_str(tfor)))
	{
		note_at(old, "prior initialisation here");
	}
}

static void excess_init(where *w, type *ty)
{
	cc1_warn_at(w, excess_init, "excess initialiser for '%s'", type_to_str(ty));
}

static decl_init *decl_init_brace_up_scalar(
		decl_init *current, init_iter *iter, type *const tfor,
		symtable *stab)
{
	decl_init *first_init;
	where *const w = ITER_WHERE(iter, &stab->where);

	if(current){
		override_warn(tfor, &current->where, w, 0);

		decl_init_free_1(current);
	}

	init_debug("brace-up-scalar: ", type_to_str(tfor));
	init_debug_dinit(iter, tfor);
	init_debug_indent(++);

	if(!iter->pos || !*iter->pos){
		first_init = decl_init_new_w(decl_init_scalar, w);
		/* default init for everything */
		first_init->bits.expr = expr_set_where(
				expr_new_val(0), w);

		FOLD_EXPR(first_init->bits.expr, stab);

		init_debug("  [zero init]\n");
		init_debug_indent(--);

		return first_init;
	}

	first_init = *iter->pos++;

	if(first_init->desig)
		die_at(&first_init->where, "initialising scalar with %s designator",
				DESIG_TO_STR(first_init->desig->type));

	if(first_init->type == decl_init_brace){
		init_iter it;
		decl_init *ret;
		unsigned n;

		init_debug("substepping through scalar...\n");

		it.pos = first_init->bits.ar.inits;

		n = dynarray_count(it.pos);
		if(n > 1)
			excess_init(&first_init->where, tfor);

		ret = decl_init_brace_up_r(current, &it, tfor, stab);

		init_debug_indent(--);

		return ret;
	}

	/* fold */
	{
		expr *e = FOLD_EXPR(first_init->bits.expr, stab);

		if(type_is_primitive(e->tree_type, type_void)){
			warn_at_print_error(&e->where, "initialisation from void expression");
			fold_had_error = 1;
		}else{
			fold_type_chk_and_cast_ty(
					tfor, &first_init->bits.expr,
					stab, &first_init->bits.expr->where,
					"initialisation");
		}

		init_debug("init scalar with %s expr\n", e->f_str());
	}

	init_debug_indent(--);

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

static void warn_replacing_with_sideeffects(where *replacing_location, decl_init *with)
{
	/* we can't check decl_init_has_sideeffects() here - may have replaced,
	 * and hence altered replacing->bits.ar.inits */

	if(cc1_warn_at(&with->where, initialiser_overrides, "initialiser with side-effects overwritten")){
		note_at(replacing_location, "overwritten initialiser here");
	}
}

static decl_init **decl_init_brace_up_array2(
		decl_init **current, struct init_cpy ***range_store,
		init_iter *iter,
		symtable *stab,
		type *next_type, const int limit,
		const int allow_struct_copy)
{
	size_t n = dynarray_count(current), i = 0;
	decl_init *this;

	(void)allow_struct_copy;

	while((this = *iter->pos)){
		desig *des;
		size_t j = i;

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
				cc1_warn_at(&des->bits.range[1]->where,
						gnu_init_array_range,
						"use of GNU array-range initialiser");

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
			where *replaced_sideeffects_location = NULL;

			if(i < n && current[i] != DYNARRAY_NULL){
				replacing = current[i]; /* replacing object `i' */

				replaced_sideeffects_location = replacing
					&& replacing != DYNARRAY_NULL
					&& decl_init_has_sideeffects(replacing)
					? &replacing->where
					: NULL;

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
				&& !type_is_scalar(next_type)){
					/* we can replace brace inits IF they're constant (as a special
					 * case), which is usually a common usage for static/global inits
					 * i.e.
					 * int x[] = { [0 ... 9] = f(), [1] = `exp' };
					 * if exp is const we can do it.
					 */
					if(!decl_init_is_const(replacing, stab, next_type, NULL, NULL)){
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
			init_debug("array init [%zu ... %zu]: ", i, j);
			init_debug_dinit(iter, next_type);
			braced = decl_init_brace_up_r(replacing, iter, next_type, stab);

			dynarray_padinsert(&current, i, &n, braced);

			if(replaced_sideeffects_location)
				warn_replacing_with_sideeffects(replaced_sideeffects_location, braced);

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

		/* [0 ... 5] leaves the current index as 6
		 *  ^i    ^j
		 */
		i = j + 1;
	}

	return current;
}

static void maybe_warn_missing_init(
		struct_union_enum_st *sue,
		unsigned i, unsigned sue_nmem,
		init_iter *iter,
		decl_init **su_inits,
		where *last_loc)
{
	unsigned diff = 0;
	unsigned si;
	decl *last_memb = NULL;

	if(sue->primitive != type_struct)
		return;
	if(i >= sue_nmem)
		return;

	for(si = i; si < sue_nmem; si++){
		decl *ent = sue->members[si]->struct_member;

		if(!DECL_IS_ANON_BITFIELD(ent)){
			diff++;
			if(!last_memb)
				last_memb = sue->members[si]->struct_member;
		}
	}

	if(diff == 1 && type_is_incomplete_array(last_memb->ref)){
		/* don't warn for flexarr */
	}else if(diff > 0){
		where *loc = ITER_WHERE(iter, last_loc ? last_loc : &sue->where);
		unsigned char *warningp = &cc1_warning.init_missing_struct;

		/* special case "= { 0 }" */
		if(i == 1 && decl_init_is_zero(su_inits[0]))
			warningp = &cc1_warning.init_missing_struct_zero;

		cc1_warn_at_w(loc,
				warningp,
				"%u missing initialiser%s for '%s %s'\n"
				"%s: note: starting at \"%s\"",
				diff, diff == 1 ? "" : "s",
				sue_str(sue), sue->spel,
				where_str(loc), last_memb->spel);
	}
}

static void decl_init_const_check(expr *e, symtable *stab)
{
	if(cc1_std <= STD_C89){
		consty k;
		fold_expr_nodecay(e, stab);
		const_fold(e, &k);

		if(!CONST_AT_COMPILE_TIME(k.type)){
			cc1_warn_at(&e->where,
					c89_init_constexpr,
					"aggregate initialiser is not a constant expression");
		}
	}
}

static decl_init **decl_init_brace_up_sue2(
		decl_init **current, decl_init ***range_store,
		init_iter *iter,
		symtable *stab,
		struct_union_enum_st *sue, const int is_anon,
		const int allow_struct_copy)
{
	size_t n = dynarray_count(current), i;
	unsigned sue_nmem;
	int had_desig = 0;
	where *first_non_desig = NULL;
	where *last_loc = NULL;
	decl_init *this;

	(void)range_store;

	UCC_ASSERT(sue_is_complete(sue), "should've checked sue completeness");

	init_debug("brace-up-sue: %s\n", sue->spel);

	/* check for copy-init */
	if(allow_struct_copy
	&& (this = *iter->pos)
	&& this->type == decl_init_scalar)
	{
		expr *e;

		fold_expr_nodecay(e = this->bits.expr, stab);

		if(type_is_s_or_u(e->tree_type) == sue){
			/* copy init */
			dynarray_padinsert(&current, 0, &n, this);

			++iter->pos;

			init_debug("sue copy-init\n");

			return current;
		}
	}

	/* check for {} */
	if(sue_sizekind(sue) != SUE_NORMAL
	&& (this = *iter->pos)
	&& (this->type != decl_init_brace
		|| dynarray_count(this->bits.ar.inits) != 0))
	{
		cc1_warn_at(&this->where, missing_empty_struct_brace_init,
				"missing {} initialiser for empty %s",
				sue_str(sue), sue->spel);
	}

	sue_nmem = sue_nmembers(sue);
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

			init_debug("sue member desig: %s\n", des->bits.member);

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
					struct_union_enum_st *jmem_sue = type_is_s_or_u(jmem->ref);
					if(jmem_sue == in){
						decl_init *replacing;
						where *replaced_sideeffects_location = NULL;

						/* anon struct/union, sub init it, restoring the desig. */
						this->desig = des;

						replacing = j < n
							&& current[j] != DYNARRAY_NULL ? current[j] : NULL;

						if(replacing && decl_init_has_sideeffects(replacing))
							replaced_sideeffects_location = &replacing->where;

						braced_sub = decl_init_brace_up_aggregate(
								replacing, iter, stab, jmem->ref,
								(aggregate_brace_f *)&decl_init_brace_up_sue2, in,
								/*anon:*/1);

						if(replaced_sideeffects_location)
							warn_replacing_with_sideeffects(replaced_sideeffects_location, braced_sub);

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
		}else{
			if(!first_non_desig)
				first_non_desig = ITER_WHERE(iter, NULL);
		}

		if(i < sue_nmem){
			sue_member *mem = sue->members[i];
			decl_init *replacing = NULL;
			where *replaced_sideeffects_location = NULL;
			decl *d_mem;

			if(!mem)
				break;
			d_mem = mem->struct_member;
			init_debug("sue member init: %s\n", decl_to_str(d_mem));

			/* skip bitfield padding
			 * init for it is <zero> created by a dynarray_padinsert */
			if(DECL_IS_ANON_BITFIELD(d_mem))
				continue;

			if(i < n && current[i] != DYNARRAY_NULL){
				replacing = current[i];

				/* check for full-object replacement, e.g.
				 * struct Sub sub;
				 * struct A a = { .obj = sub, .obj.memb = 1 };
				 *                            ^~~~~~~~~
				 * this causes the original '.obj' init to be dropped
				 */
				if(/* next designator:*/this->desig
				&& /* current designator:*/ des
				/* replacing { obj } init type: */
				&& replacing->type == decl_init_brace
				&& dynarray_count(replacing->bits.ar.inits) == 1
				&& replacing->bits.ar.inits[0]->type == decl_init_scalar
				&& type_is_s_or_u(replacing->bits.ar.inits[0]->bits.expr->tree_type))
				{
					int warned = cc1_warn_at(
							&this->where,
							init_obj_discard,
							"designating into object discards entire previous initialisation");

					if(warned)
						note_at(&replacing->where, "previous initialisation");

					/* XXX: memleak */
					replacing = NULL;
					current[i] = DYNARRAY_NULL;
				}
				else
				{
					if(decl_init_has_sideeffects(replacing))
						replaced_sideeffects_location = &replacing->where;
				}
			}

			if(type_is_incomplete_array(d_mem->ref)){
				cc1_warn_at(&this->where, flexarr_init,
						"initialisation of flexible array (GNU)");
			}

			if(!braced_sub){
				braced_sub = decl_init_brace_up_r(
						replacing, iter,
						d_mem->ref, stab);
			}

			if(replaced_sideeffects_location)
				warn_replacing_with_sideeffects(replaced_sideeffects_location, braced_sub);

			init_debug("done sue member %s\n", d_mem->spel);

			/* XXX: padinsert will insert zero inits for skipped fields,
			 * including anonymous bitfield pads
			 */
			dynarray_padinsert(&current, i, &n, braced_sub);

			/* done, check bitfield truncation */
			assert(!type_is(d_mem->ref, type_func));
			if(braced_sub && d_mem->bits.var.field_width){
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

	init_debug("done sue %s init: iter->pos = %p\n",
			sue->spel, iter ? (void *)iter->pos[0] : NULL);

	if(!had_desig)
		maybe_warn_missing_init(sue, i, sue_nmem, iter, current, last_loc);

	if(first_non_desig){
		attribute *desig_attr;

		if((desig_attr = attr_present(sue->attr, attr_desig_init))){
			char buf[WHERE_BUF_SIZ];

			cc1_warn_at(first_non_desig, init_undesignated,
				"positional initialisation of %s\n"
				"%s: note: attribute here",
				sue_str(sue),
				where_str_r(buf, &desig_attr->where));
		}
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

expr *decl_init_is_struct_copy(decl_init *di, struct_union_enum_st *constraint)
{
	decl_init *sub;
	struct_union_enum_st *su;

	if(di->type != decl_init_brace)
		return NULL;

	if(dynarray_count(di->bits.ar.inits) != 1)
		return NULL;

	sub = di->bits.ar.inits[0];
	if(sub == DYNARRAY_NULL)
		return NULL;

	if(sub->type != decl_init_scalar)
		return NULL;

	su = type_is_s_or_u(sub->bits.expr->tree_type);
	if(!su)
		return NULL;

	if(constraint && constraint != su)
		return NULL;

	return sub->bits.expr;
}

static decl_init *decl_init_brace_up_aggregate(
		decl_init *current,
		init_iter *iter,
		symtable *stab, type *tfor,
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
	const int allow_struct_copy = iter->pos
			&& iter->pos[0] && iter->pos[0]->type == decl_init_scalar;

	if(iter->pos[0]->type == decl_init_brace){
		/* pass down this as a new iterator */
		decl_init *first = iter->pos[0];
		decl_init **const braced_inits = first->bits.ar.inits;

		if(braced_inits){
			/* the brace contains some inits { 1, .x = 2, 3 } */
			init_iter it;
			decl_init *synthesized_braces[2];

			it.pos = braced_inits;

			/* prevent designator loss */
			if(first->desig){
				/* need to insert our desig
				 * e.g.
				 *
				 * .a.b = { .x = 1, 2, 3 }
				 * becomes:
				 * .a = { .b = { .x = 1, 2, 3 } }
				 */
				decl_init *synthesized_brace;

				init_debug("first desig: ");
				init_debug_desig(first->desig, stab);

				/* if we designate and pass a brace, make sure the brace-ness
				 * is passed through to the lower layers
				 *
				 * a = { .i = { 1, 2 } }
				 * need to pass { 1, 2 }, not 1, 2
				 */

				synthesized_brace = decl_init_new_w(decl_init_brace, &first->where);
				synthesized_brace->bits.ar.inits = braced_inits;
				synthesized_brace->desig = first->desig;

				synthesized_braces[0] = synthesized_brace;
				synthesized_braces[1] = NULL;

				it.pos = synthesized_braces;

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
					stab, arg1, arg2, allow_struct_copy);

			if(it.pos[0]){
				/* we know we're in a brace,
				 * so it.pos... etc aren't for anything else */
				excess_init(&it.pos[0]->where, tfor);
			}

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
				&it, stab, arg1, arg2, allow_struct_copy);

		iter->pos[desig_index] = saved;

		/* need to increment only by the amount that was used */
		iter->pos += it.pos - iter->pos;

		return ret;
	}else{
		where *loc = ITER_WHERE(iter, NULL);
		decl_init *r = decl_init_new_w(decl_init_brace, loc);
		int was_desig = !!iter->pos[0]->desig;

		/* we need to pull from iter, bracing up our children inits */
		r->bits.ar.inits = brace_up_f(
				current ? current->bits.ar.inits : NULL,
				&r->bits.ar.range_inits,
				iter, stab, arg1, arg2, allow_struct_copy);

		/* only warn if it's not designated
		 * and it's not a struct copy */
		if(!was_desig && !decl_init_is_struct_copy(r, NULL)){
			cc1_warn_at(loc,
					init_missing_braces,
					"missing braces for initialisation of sub-object '%s'",
					type_to_str(tfor));
		}

		return r;
	}
}

static void emit_incomplete_error(init_iter *iter, type *tfor)
{
	struct_union_enum_st *sue = type_is_s_or_u_or_e(tfor);

	if(sue && sue_incomplete_chk(sue, ITER_WHERE(iter, &sue->where)))
		return;

	warn_at_print_error(ITER_WHERE(iter, NULL),
			"initialising incomplete type '%s'", type_to_str(tfor));
}

static decl_init *decl_init_dummy(decl_init *current)
{
	if(current)
		return current;

	return decl_init_new(decl_init_brace);
}

static decl_init *is_char_init(
		type *ty, init_iter *iter,
		symtable *stab, int *mismatch)
{
	decl_init *this;

	if((this = *iter->pos)
	/* allow "xyz" or { "xyz" } */
	&& (this->type == decl_init_scalar
	|| (this->type == decl_init_brace &&
		  1 == dynarray_count(this->bits.ar.inits) &&
		  this->bits.ar.inits[0]->type == decl_init_scalar)))
	{
		enum type_str_type ty_expr, ty_decl;

		decl_init *chosen = this->type == decl_init_scalar
			? this : this->bits.ar.inits[0];

		fold_expr_nodecay(chosen->bits.expr, stab);

		/* only allow char[] init from string literal (or __func__)
		 * otherwise we allow things like:
		 * char x[] = (q ? "hi" : "there");
		 */
		if(!expr_kind(chosen->bits.expr, str))
			return NULL;

		ty_expr = type_str_type(chosen->bits.expr->tree_type);
		ty_decl = type_str_type(ty);

		if(ty_expr == type_str_no)
			; /* fine - not a string init */
		else if(ty_expr == ty_decl)
			return chosen;
		else if(mismatch)
			*mismatch = 1;
	}

	return NULL;
}

static decl_init *decl_init_brace_up_array_chk_char(
		decl_init *current, init_iter *iter,
		type *const next_type, symtable *stab)
{
	const int limit = type_is_incomplete_array(next_type)
		? -1 : (signed)type_array_len(next_type);

	type *array_of = type_next(next_type);

	decl_init *strk;

	init_debug("brace-up-array: of=%s\n", type_to_str(next_type));

	if(!type_is_complete(array_of)){
		emit_incomplete_error(iter, next_type);
		return decl_init_dummy(current);
	}

	if((strk = is_char_init(next_type, iter, stab, NULL))){
		consty k;

		FOLD_EXPR(strk->bits.expr, stab);
		const_fold(strk->bits.expr, &k);

		if(k.type == CONST_STRK){
			where *const w = &strk->where;
			unsigned str_i, count;
			decl_init *braced;

			if(limit == -1){
				count = k.bits.str->lit->cstr->count;
			}else{
				if(k.bits.str->lit->cstr->count <= (unsigned)limit){
					count = k.bits.str->lit->cstr->count;
				}else{
					/* only warn if it's more than one larger,
					 * i.e. allow char[2] = "hi" <-- '\0' excluded
					 */
					if(k.bits.str->lit->cstr->count - 1 > (unsigned)limit){
						cc1_warn_at(&k.bits.str->where,
								init_overlong_strliteral,
								"string literal too long for '%s'",
								type_to_str(next_type));
					}
					count = limit;
				}
			}

			braced = decl_init_new_w(decl_init_brace, w);

			for(str_i = 0; str_i < count; str_i++){
				decl_init *char_init = decl_init_new_w(decl_init_scalar, w);

				char_init->bits.expr = expr_set_where(
						expr_compiler_generated( /* use this to prevent warnings */
							expr_new_val(cstring_char_at(k.bits.str->lit->cstr, str_i))),
						&k.bits.str->where);

				FOLD_EXPR(char_init->bits.expr, stab);

				dynarray_add(&braced->bits.ar.inits, char_init);
			}

			++iter->pos;

			init_debug("array via char-init\n");

			return braced;
		}
	}

	return decl_init_brace_up_aggregate(
			current, iter, stab, next_type,
			(aggregate_brace_f *)&decl_init_brace_up_array2,
			array_of, limit);
}


static decl_init *decl_init_brace_up_r(
		decl_init *current, init_iter *iter,
		type *tfor, symtable *stab)
{
	struct_union_enum_st *sue;
	decl_init *ret;

	fold_type(tfor, stab);

	init_debug("brace-up-R: ");
	init_debug_dinit(iter, tfor);
	init_debug_indent(++);

	if(type_is(tfor, type_array)){
		ret = decl_init_brace_up_array_chk_char(
				current, iter, tfor, stab);
	}else{
		/* incomplete check _after_ array, since we allow T x[] */
		if(!type_is_complete(tfor)){
			emit_incomplete_error(iter, tfor);
			ret = decl_init_dummy(current);
			goto out;
		}

		if((sue = type_is_s_or_u(tfor))){
			ret = decl_init_brace_up_aggregate(
					current, iter, stab, tfor,
					(aggregate_brace_f *)&decl_init_brace_up_sue2,
					sue, 0 /* is anon */);
		}else{
			ret = decl_init_brace_up_scalar(current, iter, tfor, stab);
		}
	}

out:
	init_debug_indent(--);

	return ret;
}

static decl_init *decl_init_brace_up_start(
		decl_init *init, type **ptfor,
		symtable *stab)
{
	decl_init *inits[2] = {
		init, NULL
	};
	init_iter it = { inits };
	type *const tfor = *ptfor;
	decl_init *ret;
	int for_array;

	/* check for non-brace init */
	if(init
	&& init->type == decl_init_scalar
	&& ((for_array = !!type_is_array(tfor))
		|| type_is_s_or_u(tfor)))
	{
		expr *e;
		enum type_cmp cmp;

		fold_expr_nodecay(e = init->bits.expr, stab);
		cmp = type_cmp(e->tree_type, tfor, 0);

		/* allow (copy)init of const from non-const and vice versa */
		if(!(cmp & (TYPE_EQUAL_ANY | TYPE_QUAL_ADD | TYPE_QUAL_SUB))){
			/* allow special case of char [] with "..." */
			int str_mismatch = 0;

			/* if not initialising an array, type mismatch.
			 * if initialising array, but isn't char[] init, mismatch */
			if(!for_array
			|| !is_char_init(tfor, &it, stab, &str_mismatch))
			{
				fold_had_error = 1;

				warn_at_print_error(&init->where,
						str_mismatch
							? "incorrect string literal initialiser for %s"
							: "%s must be initialised with an initialiser list",
						type_to_str(tfor));
				return init;
			}else{
				e = expr_skip_lval2rval(e);
				if(expr_kind(e, str) && e->bits.strlit.is_func){
					cc1_warn_at(&init->where,
							x__func__init,
							"initialisation of %s from %s is an extension",
							type_to_str(tfor),
							e->bits.strlit.is_func == 1 ? "__func__" : "__FUNCTION__");
				}
			}
		}
		/* else struct copy init */
	}

	ret = decl_init_brace_up_r(NULL, &it, tfor, stab);

	if(type_is_incomplete_array(tfor)){
		/* complete it */
		expr *sz = expr_set_where(
				expr_new_val(dynarray_count(ret->bits.ar.inits)),
				&init->where);

		FOLD_EXPR(sz, stab); /* otherwise tree_type isn't set */

		UCC_ASSERT(ret->type == decl_init_brace, "unbraced array");
		*ptfor = type_complete_array(tfor, sz);

		init_debug("completed array type: %s\n", type_to_str(*ptfor));
	}

	return ret;
}

static void maybe_complete_array_from_prev(decl *d)
{
	if(!type_is_incomplete_array(d->ref))
		return;

	if(!d->proto)
		return;

	if(!type_is_array(d->proto->ref) || type_is_incomplete_array(d->proto->ref))
		return;

	d->ref = d->proto->ref;
	init_debug("changing %s's type to %s (from prev decl)\n",
			d->spel, type_to_str(d->ref));
}

void decl_init_brace_up_fold(decl *d, symtable *stab)
{
	init_debug("top level: %s\n", decl_to_str(d));

	assert(!type_is(d->ref, type_func));
	if(!d->bits.var.init.normalised){
		d->bits.var.init.normalised = 1;

		if(type_is_vla(d->ref, VLA_ANY_DIMENSION)){
			warn_at_print_error(
					&d->where,
					"cannot initialise variable length array");
			fold_had_error = 1;
			return;
		}

		maybe_complete_array_from_prev(d);

		d->bits.var.init.dinit = decl_init_brace_up_start(
				d->bits.var.init.dinit,
				&d->ref,
				stab);
	}
}


static expr *sue_base_for_init_assignment(
		struct_union_enum_st *sue, expr *base,
		decl **psmem, where *w,
		unsigned idx, unsigned n,
		symtable *stab)
{
	expr *e_access;
	decl *smem;

	UCC_ASSERT(idx < n, "oob member init");

	*psmem = smem = sue->members[idx]->struct_member;

	/* don't create zero-width bitfield inits */
	if(DECL_IS_ANON_BITFIELD(smem)
	&& const_expr_and_zero(smem->bits.var.field_width))
	{
		return NULL;
	}

	e_access = expr_set_where(
			expr_new_struct_mem(base, 1, smem),
			w);

	fold_expr_nodecay(e_access, stab);

	return e_access;
}

static void expr_init_add(expr **pinit, expr *new, symtable *stab)
{
	fold_expr_nodecay(new, stab);

	if(*pinit){
		*pinit = expr_set_where(expr_new_comma2(*pinit, new, 1), &new->where);
		fold_expr_nodecay(*pinit, stab);
	}else{
		*pinit = new;
	}
}

static void decl_init_create_assignment_from_copy(
		decl_init *di, expr **pinit,
		type *next_type, expr *new_base,
		symtable *stab)
{
	/* TODO: ideally when the backend is sufficiently optimised
	 * it'll pick it up the memcpy well
	 */
	struct init_cpy *icpy = *di->bits.range_copy;

	UCC_ASSERT(next_type, "no next type for array");

	/* memcpy from the previous init */
	if(icpy->first_instance){
		expr *last_base = icpy->first_instance;

		expr *memcp = expr_compiler_generated(
				builtin_new_memcpy(
					new_base, last_base, type_size(next_type, &di->where)));

		expr_init_add(pinit, memcp, stab);
	}else{
		/* the initial assignment from the range_copy */
		icpy->first_instance = new_base;

		decl_init_create_assignments_base(icpy->range_init,
				next_type, new_base, pinit, stab, 1);
	}
}

void decl_init_create_assignments_base(
		decl_init *init,
		type *tfor, expr *base,
		expr **pinit,
		symtable *stab,
		const int aggregate)
{
	if(!init || decl_init_is_zero_fold(init, stab)){
		expr *zero;

zero_init:
		if(type_is_incomplete_array(tfor) || type_is_variably_modified(tfor)){
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
				type_size(tfor, &base->where));

		memcpy_safe(&zero->where, &base->where);

		expr_init_add(pinit, zero, stab);
		return;
	}

	switch(init->type){
		case decl_init_scalar:
			if(aggregate)
				decl_init_const_check(init->bits.expr, stab);

			expr_init_add(pinit,
					expr_set_where(
						expr_new_assign_init(base, init->bits.expr),
						&base->where),
					stab);
			break;

		case decl_init_copy:
			ICE("copy got through assignment");

		case decl_init_brace:
		{
			struct_union_enum_st *sue = type_is_s_or_u(tfor);
			size_t n;
			decl_init **i;
			unsigned idx;

			if(sue /* check for struct copy */
			&& dynarray_count(init->bits.ar.inits) == 1
			&& init->bits.ar.inits[0] != DYNARRAY_NULL
			&& init->bits.ar.inits[0]->type == decl_init_scalar)
			{
				expr *e = init->bits.ar.inits[0]->bits.expr;

				if(type_is_s_or_u(e->tree_type) == sue){
					decl_init_const_check(e, stab);

					expr_init_add(pinit,
							builtin_new_memcpy(
								base, e, type_size(e->tree_type, &e->where)),
							stab);
					return;
				}
			}

			/* type_array_len()
			 * we're already braced so there are no incomplete arrays
			 * except for C99 flexible arrays
			 */
			if(sue){
				n = dynarray_count(sue->members);
			}else if(type_is_incomplete_array(tfor)){
				n = dynarray_count(init->bits.ar.inits);

				/* it's fine if there's nothing for it */
				if(n > 0)
					die_at(&init->where, "non-static initialisation of flexible array");
			}else if(type_is_variably_modified(tfor)){
				/* error already emitted */
				return;
			}else{
				if(!type_is_complete(tfor)){
					warn_at_print_error(
							&init->where,
							"initialising incomplete type '%s'",
							type_to_str(tfor));
					fold_had_error = 1;
					return;
				}
				n = type_array_len(tfor);
			}

			/* check union */
			if(sue && sue->primitive == type_union){
				decl *smem;
				expr *sue_base;

				/* look for a non null init */
				for(idx = 0, i = init->bits.ar.inits; *i == DYNARRAY_NULL; i++, idx++);

				if(*i){
					sue_base = sue_base_for_init_assignment(
							sue, base, &smem, &init->where, idx, n, stab);

					UCC_ASSERT(sue_base, "zero width bitfield init in union?");

					decl_init_create_assignments_base(
							*i,
							smem->ref,
							sue_base,
							pinit,
							stab, 1);
				}else{
					/* zero init union - make sure we get all of it */
					goto zero_init;
				}
				return;
			}

			for(idx = 0, i = init->bits.ar.inits; idx < n; (*i ? i++ : 0), idx++){
				decl_init *di = *i;
				expr *new_base;
				type *next_type = NULL;

				if(di == DYNARRAY_NULL)
					di = NULL;

				if(sue){
					decl *smem = sue->members[idx]->struct_member;

					UCC_ASSERT(sue->primitive != type_union, "sneaky union");

					new_base = sue_base_for_init_assignment(
							sue, base, &smem, di ? &di->where : &init->where, idx, n,
							stab);

					if(!new_base)
						continue; /* 0-width bitfield */

					next_type = smem->ref;

				}else{
					/* array case */
					new_base = expr_set_where(
							expr_new_array_idx(base, idx),
							&base->where);

					fold_expr_nodecay(new_base, stab);

					if(!next_type)
						next_type = type_next(tfor);

					if(di && di != DYNARRAY_NULL && di->type == decl_init_copy){
						decl_init_create_assignment_from_copy(
								di, pinit, next_type, new_base, stab);
						continue;
					}
				}

				decl_init_create_assignments_base(
						di, next_type,
						new_base, pinit,
						stab, 1);
			}
			break;
		}
	}
}

void decl_init_create_assignments_base_and_fold(
		decl *d, expr *e, symtable *scope)
{
	decl_init_create_assignments_base(d->bits.var.init.dinit,
			d->ref, e, &d->bits.var.init.expr, scope,
			!type_is_scalar(d->ref));

	if(d->bits.var.init.expr)
		FOLD_EXPR(d->bits.var.init.expr, scope);
	/* else had error */
}

void decl_default_init(decl *d, symtable *stab)
{
	assert(!type_is(d->ref, type_func));

	UCC_ASSERT(!d->bits.var.init.dinit, "already initialised?");

	if(type_is_variably_modified(d->ref)){
		/* error emitted elsewhere */
		return;
	}

	d->bits.var.init.dinit = decl_init_new_w(decl_init_brace, &d->where);
	d->bits.var.init.compiler_generated = 1;
	decl_init_brace_up_fold(d, stab);
}
