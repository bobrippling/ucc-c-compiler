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

typedef decl_init **aggregate_brace_f(decl_init **, init_iter *, void *, int);

static decl_init *decl_init_brace_up_aggregate(
		decl_init *current,
		init_iter *iter,
		aggregate_brace_f *,
		void *arg1, int arg2);

int decl_init_is_const(decl_init *dinit, symtable *stab)
{
	switch(dinit->type){
		case decl_init_scalar:
		{
			expr *e;
			consty k;

			e = FOLD_EXPR(dinit->bits.expr, stab);
			const_fold(e, &k);

			return is_const(k.type);
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

/*static expr *expr_new_array_idx_e(expr *base, expr *idx)
{
	expr *op = expr_new_op(op_plus);
	op->lhs = base;
	op->rhs = idx;
	return expr_new_deref(op);
}

static expr *expr_new_array_idx(expr *base, int i)
{
	return expr_new_array_idx_e(base, expr_new_val(i));
}*/

/*
 * brace up code
 * -------------
 */

static decl_init *decl_init_brace_up(decl_init *current, init_iter *, type_ref *);

#define DINIT_STR(t) (const char *[]){"scalar","brace"}[t]


static decl_init *decl_init_brace_up_scalar(
		decl_init *current, init_iter *iter, type_ref *const tfor)
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

	if(!iter->pos){
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
		return decl_init_brace_up(current, &it, tfor);
	}

	return first_init;
}

static decl_init **decl_init_brace_up_array2(
		decl_init **current, init_iter *iter,
		type_ref *next_type,
		const int limit)
{
	unsigned n = dynarray_count(current), i = 0;
	decl_init *this;

	while((this = *iter->pos)){
		desig *des;

		if(limit > -1 && i >= (unsigned)limit)
			break;

		if((des = this->desig)){
			consty k;

			this->desig = des->next;

			if(des->type != desig_ar){
				DIE_AT(&this->where,
						"%s designator can't designate array",
						DESIG_TO_STR(des->type));
			}

			const_fold(des->bits.ar, &k);

			if(k.type != CONST_VAL)
				DIE_AT(&this->where, "non-constant array-designator");

			i = k.bits.iv.val;
			if(limit > -1 && i >= (unsigned)limit)
				DIE_AT(&this->where, "designating outside of array bounds (%d)", limit);
		}

		{
			decl_init *replacing = NULL;

			if(i < n && current[i] != DYNARRAY_NULL)
				replacing = current[i]; /* replacing object `i' */

			dynarray_padinsert(&current, i, &n,
					decl_init_brace_up(replacing, iter, next_type));
		}

		i++;
	}

	return current;
}

static decl_init **decl_init_brace_up_sue2(
		decl_init **current, init_iter *iter,
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

			if(j)
				ICE("Plan 9 struct init TODO");

			for(j = 0; sue->members[j]; j++){
				decl *jmem = sue->members[j]->struct_member;

				if(jmem == mem){
					found = 1;
				}else if(!jmem->spel && in){
					struct_union_enum_st *jmem_sue = type_ref_is_s_or_u(jmem->ref);
					if(jmem_sue == in){
						/* anon struct/union, sub init it, restoring the desig. */
						this->desig = des;
						ICW("TODO: replacements");
						braced_sub = decl_init_brace_up_aggregate(
								NULL /* FIXME/replace */, iter,
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
				braced_sub = decl_init_brace_up(
						replacing,
						iter, mem->struct_member->ref);
			}

			dynarray_padinsert(&current, i, &n, braced_sub);
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
					&it, arg1, arg2);

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
				&it, arg1, arg2);

		*(iter->pos += desig_index) = saved;

		return ret;
	}else{
		decl_init *r = decl_init_new(decl_init_brace);

		/* we need to pull from iter, bracing up our children inits */
		r->bits.inits = brace_up_f(
				current ? current->bits.inits : NULL,
				iter, arg1, arg2);

		return r;
	}
}

static decl_init *decl_init_brace_up(decl_init *current, init_iter *iter, type_ref *tfor)
{
	struct_union_enum_st *sue;

	if(type_ref_is(tfor, type_ref_array)){
		const int limit = type_ref_is_incomplete_array(tfor)
			? -1 : type_ref_array_len(tfor);
		type_ref *next = type_ref_next(tfor);

		if(!type_ref_is_complete(next))
			goto bad_init;

		return decl_init_brace_up_aggregate(
				current, iter,
				(aggregate_brace_f *)&decl_init_brace_up_array2,
				type_ref_next(tfor), limit);
	}

	/* incomplete check _after_ array, since we allow T x[] */
	if(!type_ref_is_complete(tfor)){
bad_init:
		DIE_AT(ITER_WHERE(iter, &tfor->where),
				"initialising %s", type_ref_to_str(tfor));
	}

	if((sue = type_ref_is_s_or_u(tfor)))
		return decl_init_brace_up_aggregate(
				current, iter,
				(aggregate_brace_f *)&decl_init_brace_up_sue2,
				sue, 0 /* is anon */);

	return decl_init_brace_up_scalar(current, iter, tfor);
}

static void DEBUG(decl_init *init, type_ref *tfor)
{
	decl_init *inits[2] = {
		init, NULL
	};
	init_iter it;
	it.pos = inits;

	decl_init *braced = decl_init_brace_up(NULL, &it, tfor);

	cc1_out = stdout;
	printf("braced = %p\n", (void *)braced);
	void print_decl_init(void *);
	print_decl_init(braced);

	exit(5);
}

void decl_init_create_assignments_for_spel(decl *d, stmt *init_code)
{
	(void)init_code;
	DEBUG(d->init, d->ref);
}

void decl_init_create_assignments_for_base(decl *d, expr *base, stmt *init_code)
{
	(void)init_code;
	(void)base;
	DEBUG(d->init, d->ref);
}
