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

#define ITER_WHERE(it, def) (it && it->pos[0] ? &it->pos[0]->where : def)

typedef decl_init **aggregate_brace_f(decl_init **, init_iter *,
		symtable *,
		void *, int);

static decl_init *decl_init_brace_up_aggregate(
		decl_init *current,
		init_iter *iter,
		symtable *stab,
		aggregate_brace_f *,
		void *arg1, int arg2);

/* null init are const/zero, flag-init is const/zero if prev. is const/zero,
 * which will be checked elsewhere */
#define DINIT_NULL_CHECK(di)                        \
	if(di == DYNARRAY_FLAG || di == DYNARRAY_NULL)    \
		return 1

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

			for(i = dinit->bits.inits; i && *i; i++)
				if(!decl_init_is_const(*i, stab))
					return 0;

			return 1;
		}
	}

	ICE("bad decl init");
	return -1;
}

int decl_init_is_zero(decl_init *dinit)
{
	DINIT_NULL_CHECK(dinit);

	switch(dinit->type){
		case decl_init_scalar:
		{
			consty k;

			const_fold(dinit->bits.expr, &k);

			return k.type == CONST_VAL && k.bits.iv.val == 0;
		}

		case decl_init_brace:
		{
			decl_init **i;

			for(i = dinit->bits.inits; i && *i; i++)
				if(!decl_init_is_zero(*i))
					return 0;

			return 1;
		}
	}

	ICE("bad decl init");
	return -1;
}

decl_init *decl_init_new(enum decl_init_type t)
{
	decl_init *di = umalloc(sizeof *di);
	where_new(&di->where);
	di->type = t;
	return di;
}

void decl_init_free_1(decl_init *di)
{
	free(di);
}

const char *decl_init_to_str(enum decl_init_type t)
{
	switch(t){
		CASE_STR_PREFIX(decl_init, scalar);
		CASE_STR_PREFIX(decl_init, brace);
	}
	return NULL;
}

/*
 * brace up code
 * -------------
 */

static decl_init *decl_init_brace_up_r(decl_init *current, init_iter *, type_ref *, symtable *stab);


static decl_init *decl_init_brace_up_scalar(
		decl_init *current, init_iter *iter, type_ref *const tfor,
		symtable *stab)
{
	decl_init *first_init;

	if(current){
		char buf[WHERE_BUF_SIZ];

		WARN_AT(ITER_WHERE(iter, &tfor->where),
				"overriding initialisation of \"%s\"\n"
				"%s prior initialisation here",
				type_ref_to_str(tfor),
				where_str_r(buf, &current->where));

		decl_init_free_1(current);
	}

	if(!iter->pos || !*iter->pos){
		first_init = decl_init_new(decl_init_scalar);
		first_init->bits.expr = expr_new_val(0); /* default init for everything */
		return first_init;
	}

	first_init = *iter->pos++;

	if(first_init->desig)
		DIE_AT(&first_init->where, "initialising scalar with %s designator",
				DESIG_TO_STR(first_init->desig->type));

	if(first_init->type == decl_init_brace){
		init_iter it;
		it.pos = first_init->bits.inits;
		return decl_init_brace_up_r(current, &it, tfor, stab);
	}

	/* fold */
	{
		char buf[TYPE_REF_STATIC_BUFSIZ];
		expr *e = FOLD_EXPR(first_init->bits.expr, stab);

		if(!fold_type_ref_equal(
					tfor, e->tree_type, &first_init->where,
					WARN_ASSIGN_MISMATCH,
					DECL_CMP_ALLOW_VOID_PTR | DECL_CMP_ALLOW_SIGNED_UNSIGNED,
					"mismatching types in initialisation (%s <-- %s)",
					type_ref_to_str_r(buf, tfor), type_ref_to_str(e->tree_type)))
		{
			expr *cast = expr_new_cast(tfor, 1);
			cast->expr = first_init->bits.expr;
			first_init->bits.expr = cast;
		}
	}

	return first_init;
}

static decl_init **decl_init_brace_up_array2(
		decl_init **current, init_iter *iter,
		symtable *stab,
		type_ref *next_type, const int limit)
{
	unsigned n = dynarray_count(current), i = 0, j = 0;
	decl_init *this;

	while((this = *iter->pos)){
		desig *des;

		if(limit > -1 && i >= (unsigned)limit)
			break;

		if((des = this->desig)){
			consty k[2];

			this->desig = des->next;

			if(des->type != desig_ar){
				DIE_AT(&this->where,
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

			if(k[0].type != CONST_VAL || k[1].type != CONST_VAL)
				DIE_AT(&this->where, "non-constant array-designator");

			if(k[0].bits.iv.val < 0 || k[1].bits.iv.val < 0)
				DIE_AT(&this->where, "negative array index initialiser");

			if(limit > -1
			&& (k[0].bits.iv.val >= (long)limit
			||  k[1].bits.iv.val >= (long)limit))
			{
				DIE_AT(&this->where, "designating outside of array bounds (%d)", limit);
			}

			i = k[0].bits.iv.val;
			j = k[1].bits.iv.val;
		}

		{
			decl_init *replacing = NULL;
			unsigned replace_idx;
			decl_init *braced;

			if(i < n && current[i] != DYNARRAY_NULL)
				replacing = current[i]; /* replacing object `i' */

			/* check for char[] init */
			braced = decl_init_brace_up_r(replacing, iter, next_type, stab);

			dynarray_padinsert(&current, i, &n, braced);

			for(replace_idx = i + 1; replace_idx <= j; replace_idx++)
				replacing = dynarray_padinsert(&current, replace_idx, &n,
						(decl_init *)DYNARRAY_FLAG);
		}

		i++;
		j++;
	}

	return current;
}

static decl_init **decl_init_brace_up_sue2(
		decl_init **current, init_iter *iter,
		symtable *stab,
		struct_union_enum_st *sue, const int is_anon)
{
	unsigned n = dynarray_count(current), i;
	decl_init *this;

	UCC_ASSERT(sue->members, "no members in struct");

	for(i = 0; (this = *iter->pos); i++){
		desig *des;
		decl_init *braced_sub = NULL;

		if((des = this->desig)){
			/* find member, set `i' to its index */
			struct_union_enum_st *in;
			decl *mem;
			unsigned j = 0;
			int found = 0;

			if(des->type != desig_struct){
				DIE_AT(&this->where,
						"%s designator can't designate struct",
						DESIG_TO_STR(des->type));
			}

			this->desig = des->next;

			mem = struct_union_member_find(sue, des->bits.member, &j, &in);
			if(!mem){
				/* if we're an anonymous struct, return out */
				if(is_anon){
					this->desig = des;
					break;
				}

				DIE_AT(&this->where,
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
								replacing, iter, stab,
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

		{
			sue_member *mem = sue->members[i];
			decl_init *replacing = NULL;

			if(!mem)
				break;

			if(i < n && current[i] != DYNARRAY_NULL)
				replacing = current[i];

			if(!braced_sub){
				braced_sub = decl_init_brace_up_r(
						replacing, iter,
						mem->struct_member->ref, stab);
			}

			dynarray_padinsert(&current, i, &n, braced_sub);

			if(sue->primitive == type_union)
				break;
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

static decl_init *decl_init_brace_up_aggregate(
		decl_init *current,
		init_iter *iter,
		symtable *stab,
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
		decl_init **old_subs = first->bits.inits;

		if(old_subs){
			init_iter it;

			it.pos = old_subs;

			first->bits.inits = brace_up_f(
					current ? current->bits.inits : NULL,
					&it,
					stab, arg1, arg2);

			free(old_subs);

		}else{
			/* {} */
			first->bits.inits = NULL;
			dynarray_add(&first->bits.inits, (decl_init *)DYNARRAY_NULL);
		}

		++iter->pos;
		return first; /* this is in the {} state */

	}else if((desig_index = find_desig(iter->pos)) > 0){
		decl_init *const saved = iter->pos[desig_index];
		decl_init *ret = decl_init_new(decl_init_brace);
		init_iter it = { iter->pos };

		iter->pos[desig_index] = NULL;

		ret->bits.inits = brace_up_f(
				current ? current->bits.inits : NULL,
				&it, stab, arg1, arg2);

		*(iter->pos += desig_index) = saved;

		return ret;
	}else{
		decl_init *r = decl_init_new(decl_init_brace);

		/* we need to pull from iter, bracing up our children inits */
		r->bits.inits = brace_up_f(
				current ? current->bits.inits : NULL,
				iter, stab, arg1, arg2);

		return r;
	}
}

static void die_incomplete(init_iter *iter, type_ref *tfor)
{
	DIE_AT(ITER_WHERE(iter, &tfor->where),
			"initialising %s", type_ref_to_str(tfor));
}

static decl_init *decl_init_brace_up_array_pre(
		decl_init *current, init_iter *iter,
		type_ref *next_type, symtable *stab)
{
	const int limit = type_ref_is_incomplete_array(next_type)
		? -1 : type_ref_array_len(next_type);

	type_ref *next = type_ref_next(next_type);

	const int for_str_ar = !!type_ref_is_type(
			type_ref_is_array(next_type), type_char);

	decl_init *this;

	if(!type_ref_is_complete(next))
		die_incomplete(iter, next_type);

	if(for_str_ar
	&& (this = *iter->pos)
	/* allow "xyz" or { "xyz" } */
	&& (this->type == decl_init_scalar
	|| (this->type == decl_init_brace &&
		  1 == dynarray_count(this->bits.inits) &&
		  this->bits.inits[0]->type == decl_init_scalar)))
	{
		decl_init *strk = this->type == decl_init_scalar
			? this : this->bits.inits[0];

		consty k;

		FOLD_EXPR(strk->bits.expr, stab);
		const_fold(strk->bits.expr, &k);

		if(k.type == CONST_STRK){
			unsigned str_i;

			if(k.bits.str->wide)
				ICE("TODO: wide string init");

			decl_init *braced = decl_init_new(decl_init_brace);

			for(str_i = 0; str_i < k.bits.str->len; str_i++){
				decl_init *char_init = decl_init_new(decl_init_scalar);

				char_init->bits.expr = expr_new_val(k.bits.str->str[str_i]);

				dynarray_add(&braced->bits.inits, char_init);
			}

			++iter->pos;

			return braced;
		}
	}

	return decl_init_brace_up_aggregate(
			current, iter, stab,
			(aggregate_brace_f *)&decl_init_brace_up_array2,
			type_ref_next(next_type), limit);
}


static decl_init *decl_init_brace_up_r(
		decl_init *current, init_iter *iter,
		type_ref *tfor, symtable *stab)
{
	struct_union_enum_st *sue;

	if(type_ref_is(tfor, type_ref_array))
		return decl_init_brace_up_array_pre(
				current, iter, tfor, stab);

	/* incomplete check _after_ array, since we allow T x[] */
	if(!type_ref_is_complete(tfor))
		die_incomplete(iter, tfor);

	if((sue = type_ref_is_s_or_u(tfor)))
		return decl_init_brace_up_aggregate(
				current, iter, stab,
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

	/* check for non-brace init */
	if(type_ref_is_s_or_u(tfor)
	&& init
	&& init->type == decl_init_scalar)
	{
		/* TODO: struct copy init */
		DIE_AT(&init->where,
				"%s must be initialised with an initialiser list",
				type_ref_to_str(tfor));
	}

	ret = decl_init_brace_up_r(NULL, &it, tfor, stab);

	if(type_ref_is_incomplete_array(tfor)){
		/* complete it */
		UCC_ASSERT(ret->type == decl_init_brace, "unbraced array");
		*ptfor = type_ref_complete_array(tfor, dynarray_count(ret->bits.inits));
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

	return expr_new_struct(
			base,
			1 /* . */,
			expr_new_identifier(smem->spel));
}

void decl_init_create_assignments_base(
		decl_init *init,
		type_ref *tfor, expr *base,
		stmt *code)
{
	if(!init){
		expr *zero;

zero_init:
		zero = builtin_new_memset(
				expr_new_addr(base),
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

		case decl_init_brace:
		{
			struct_union_enum_st *sue = type_ref_is_s_or_u(tfor);
			const size_t n = sue ? dynarray_count(sue->members) : type_ref_array_len(tfor);
			decl_init **i, *last_nonflag = NULL;
			unsigned idx;
			expr *last_base = NULL;

			if(sue && sue->primitive == type_union){
				decl *smem;
				expr *sue_base;

				/* look for a non null init */
				for(idx = 0, i = init->bits.inits; *i == DYNARRAY_NULL; i++, idx++);

				if(*i){
					UCC_ASSERT(*i != DYNARRAY_FLAG, "range init for union");

					sue_base = decl_init_create_assignments_sue_base(
							sue, base, &smem, idx, n);

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

			for(idx = 0, i = init->bits.inits; idx < n; (*i ? i++ : 0), idx++){
				decl_init *di = *i;
				expr *new_base;
				type_ref *next_type = NULL;

				if(di == DYNARRAY_NULL)
					di = NULL;

				if(sue){
					decl *smem;

					UCC_ASSERT(di != DYNARRAY_FLAG, "range init for struct");
					UCC_ASSERT(sue->primitive != type_union, "sneaky union");

					new_base = decl_init_create_assignments_sue_base(
							sue, base, &smem, idx, n);

					next_type = smem->ref;
				}else{
					new_base = expr_new_array_idx(base, idx);

					if(!next_type)
						next_type = type_ref_next(tfor);

					if(di == DYNARRAY_FLAG){
						UCC_ASSERT(last_nonflag, "no previous init for '...'");

						/* TODO: ideally when the backend is sufficiently optimised, we
						 * will always be able to use the memcpy case, and it'll pick it up
						 */
						if(last_nonflag->type == decl_init_scalar){
							unsigned n = dynarray_count(code->codes);
							stmt *last_assign;
							UCC_ASSERT(n > 0, "bad range init - no previous exprs");

							last_assign = code->codes[n - 1];

							/* insert like so:
							 *
							 * this = (prev_assign = ...)
							 *        ^-----------------^
							 *          already present
							 */
							last_assign->expr = expr_new_assign_init(new_base, last_assign->expr);
						}else{
							/* memcpy from the previous init */
							expr *memcp;

							UCC_ASSERT(next_type, "no next type for array (i=%d)", idx);
							UCC_ASSERT(last_base, "no previous base for ... init");

							memcp = builtin_new_memcpy(
									new_base, last_base, type_ref_size(next_type, &di->where));

							dynarray_add(&code->codes,
									expr_to_stmt(memcp, code->symtab));
						}
						continue;
					}

					last_nonflag = di;
				}

				decl_init_create_assignments_base(di, next_type, new_base, code);
				last_base = new_base;
			}
			break;
		}
	}
}
