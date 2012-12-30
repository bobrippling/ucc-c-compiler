#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>
#include <limits.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "cc1.h"
#include "fold.h"
#include "const.h"
#include "macros.h"
#include "sue.h"

#include "decl_init.h"

#if 0
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
#  define DEBUG_DECL_INIT 1
#else
#  define INIT_DEBUG_DEPTH(op)
#  define INIT_DEBUG(...)
#endif

static void decl_init_create_assignments_discard(
		decl_init ***init_iter,
		type_ref *const tfor_wrapped,
		expr *base,
		stmt *init_code);

int decl_init_len(decl_init *di)
{
 switch(di->type){
	 case decl_init_scalar:
		 return 1;

	 case decl_init_brace:
		 return dynarray_count((void **)di->bits.inits);
 }
 ICE("decl init bad type");
 return -1;
}

int decl_init_is_const(decl_init *dinit, symtable *stab)
{
	switch(dinit->type){
		case decl_init_scalar:
		{
			expr *e;
			consty k;

			e = FOLD_EXPR(dinit->bits.expr, stab);
			const_fold(e, &k);

			return k.type != CONST_NO;
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

static expr *expr_new_array_idx(expr *base, int i)
{
	expr *op = expr_new_op(op_plus);
	op->lhs = base;
	op->rhs = expr_new_val(i);
	return expr_new_deref(op);
}

static type_ref *decl_initialise_array(
		decl_init ***init_iter,
		type_ref *const tfor_wrapped, expr *base, stmt *init_code)
{
	decl_init *dinit = **init_iter;
	type_ref *tfor_deref, *tfor;
	int complete_to = 0;

	switch(dinit ? dinit->type : decl_init_brace){
		case decl_init_scalar:
			/* if we're nested, pull as many as we need from the init */
			/* FIXME: can't check tree_type - fold hasn't occured yet */
			if((tfor = type_ref_is(dinit->bits.expr->tree_type, type_ref_ptr))
					&& type_ref_is_type(tfor->ref, type_char))
			{
				/* const char * init - need to check tfor is of the same type */
				ICE("TODO: array init with string literal");
			}

			/* check for scalar init isn't done here */
			break;

		case decl_init_brace:
			break;
	}

	tfor = tfor_wrapped;
	tfor_deref = type_ref_is(tfor_wrapped, type_ref_array)->ref;

	/* walk through the inits, pulling as many as we need/one sub-brace for a sub-init */
	/* e.g.
	 * int x[]    = { 1, 2, 3 };    subinit = int,    pull 1 for each
	 * int x[][2] = { 1, 2, 3, 4 }  subinit = int[2], pull 2 for each
	 *
	 * int x[][2] = { {1}, 2, 3, {4}, 5, {6}, {7} };
	 * subinit = int[2], pull as show:
	 *              { {1}, 2, 3, {4}, 5, {6}, {7} };
	 *                 ^   ^--^   ^   ^   ^    ^      -> 6 inits
	 */

	if(dinit){
		decl_init **array_iter = (dinit->type == decl_init_scalar ? *init_iter : dinit->bits.inits);
		const int known_length = !type_ref_is_incomplete_array(tfor);
		const int lim = known_length ? type_ref_array_len(tfor) : INT_MAX;
		int i;
#ifdef DEBUG_DECL_INIT
		decl_init **start = array_iter;
#endif

		INIT_DEBUG("initialising array from %s\n", decl_init_to_str(dinit->type));
		INIT_DEBUG_DEPTH(++);

		for(i = 0; array_iter && *array_iter && i < lim; i++){
			/* index into the main-array */
			expr *this = expr_new_array_idx(base, i);

			INIT_DEBUG("initialising (%s)[%d] with %s\n",
					type_ref_to_str(tfor), i,
					decl_init_to_str((*array_iter)->type));

			INIT_DEBUG_DEPTH(++);
			decl_init_create_assignments_discard(
					&array_iter, tfor_deref, this, init_code);
			INIT_DEBUG_DEPTH(--);
		}

		if(known_length){
			/* need to zero-fill */
			for(; i < lim; i++){
				expr *this = expr_new_array_idx(base, i);

				decl_init_create_assignments_discard(
						&array_iter /* ptr to null */,
						tfor_deref, this, init_code);
			}
		}

		complete_to = i;
		*init_iter += complete_to; /* advance by the number of steps we moved over */

		INIT_DEBUG_DEPTH(--);
		INIT_DEBUG(
				"array, len %d finished, i=%d, "
				"*array_iter=%p, array_iter-start = %ld <-- adv-by\n",
				complete_to, i,
				(void *)*array_iter, (long)(array_iter - start));
	}

	/* patch the type size */
	if(type_ref_is_incomplete_array(tfor)){
		tfor = type_ref_complete_array(tfor, complete_to);

		INIT_DEBUG("completed array to %d - %s\n",
				complete_to, type_ref_to_str(tfor));
	}

	return tfor;
}

static void decl_initialise_sue(decl_init ***init_iter,
		struct_union_enum_st *sue, expr *base, stmt *init_code)
{
	/* iterate over each member, pulling from the dinit */
	sue_member **smem;
	decl_init *dinit = *init_iter ? **init_iter : NULL;
	decl_init **sue_iter;
	int braced;
	int cnt;

	if(dinit == NULL)
		ICE("TODO: null dinit for struct");

	if(sue_incomplete(sue)){
		type_ref *r = type_ref_new_type(type_new_primitive(type_struct));
		r->bits.type->sue = sue;

		DIE_AT(&dinit->where, "initialising %s", type_ref_to_str(r));
	}

	braced = dinit->type == decl_init_brace;
	sue_iter = braced ? dinit->bits.inits : *init_iter;

	for(smem = sue->members, cnt = 0;
			smem && *smem;
			smem++, cnt++)
	{
		decl *const sue_mem = (*smem)->struct_member;

		/* XXX: room for optimisation below - avoid sue name lookup */
		expr *accessor = expr_new_struct(base, 1 /* a.b */,
				expr_new_identifier(sue_mem->spel));

		decl_init_create_assignments_discard(
				&sue_iter,
				sue_mem->ref,
				accessor,
				init_code);
	}

	if(braced)
		cnt = 1; /* we walk over the one brace, not multiple scalar/subinits */

	*init_iter += cnt;
	INIT_DEBUG("initialised %s, *init_iter += %d -> %p (%s)\n",
			sue_str(sue), cnt, (void *)*init_iter,
			*init_iter && **init_iter
			? decl_init_to_str((**init_iter)->type)
			: "n/a");
}

static void decl_initialise_scalar(
		decl_init ***init_iter, expr *base, stmt *init_code)
{
	decl_init *const dinit = *init_iter ? **init_iter : NULL;
	expr *assign_from, *assign_init;

	if(dinit){
		if(dinit->type == decl_init_brace){
			/* initialising scalar with { ... } - pick first */
			decl_init **inits = dinit->bits.inits;

			if(inits && inits[1])
				WARN_AT(&inits[1]->where, "excess initaliser%s", inits[2] ? "s" : "");

			/* this seems to be called when it shouldn't... */
			decl_initialise_scalar(&inits, base, init_code);
			goto fin;
		}

		assert(dinit->type == decl_init_scalar);
		assign_from = dinit->bits.expr;
	}else{
		assign_from = expr_new_val(0);
	}

	assign_init = expr_new_assign(base, assign_from);
	assign_init->assign_is_init = 1;

	dynarray_add((void ***)&init_code->codes,
			expr_to_stmt(assign_init, init_code->symtab));

	if(dinit)
fin: ++*init_iter; /* we've used this init */
}

static type_ref *decl_init_create_assignments(
		decl_init ***init_iter,
		type_ref *const tfor_wrapped, /* could be typedef/cast */
		expr *base,
		stmt *init_code)
{
	/* iterate over tfor's array/struct members/scalar,
	 * pulling from dinit as necessary */
	type_ref *tfor, *tfor_ret = tfor_wrapped;
	struct_union_enum_st *sue;

	if((tfor = type_ref_is(tfor_wrapped, type_ref_array))){
		tfor_ret = decl_initialise_array(init_iter, tfor, base, init_code);

	}else if((sue = type_ref_is_s_or_u(tfor_wrapped))){
		decl_initialise_sue(init_iter, sue, base, init_code);

	}else{
		decl_initialise_scalar(init_iter, base, init_code);
	}

	return tfor_ret;
}

static void decl_init_create_assignments_discard(
		decl_init ***init_iter, type_ref *const tfor_wrapped,
		expr *base, stmt *init_code)
{
	type_ref *t = decl_init_create_assignments(init_iter, tfor_wrapped, base, init_code);

	if(t != tfor_wrapped)
		type_ref_free_1(t);
}

static type_ref *decl_init_create_assignments_from_init(
		decl_init *single_init,
		type_ref *const tfor_wrapped, /* could be typedef/cast */
		expr *base,
		stmt *init_code)
{
	decl_init *ar[] = { single_init, NULL };
	decl_init **it = ar;
	struct_union_enum_st *sue;

	/* init validity checks */
	if(single_init->type == decl_init_scalar){
		if((sue = type_ref_is_s_or_u(tfor_wrapped))
		||        type_ref_is(       tfor_wrapped, type_ref_array))
		{
			DIE_AT(&single_init->where, "%s must be initalised with an initialiser list",
					sue ? sue_str(sue) : "array");
		}
	}


	return decl_init_create_assignments(
			&it, tfor_wrapped, base, init_code);
}

void decl_init_create_assignments_for_spel(decl *d, stmt *init_code)
{
	d->ref = decl_init_create_assignments_from_init(
			d->init, d->ref,
			expr_new_identifier(d->spel), init_code);
}

void decl_init_create_assignments_for_base(decl *d, expr *base, stmt *init_code)
{
	d->ref = decl_init_create_assignments_from_init(
			d->init, d->ref, base, init_code);
}
