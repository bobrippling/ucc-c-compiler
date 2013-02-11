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


int decl_init_is_const(decl_init *dinit, symtable *stab)
{
	desig *desig;

	for(desig = dinit->desig; desig; desig = desig->next)
		if(desig->type == desig_ar){
			consty k;
			const_fold(desig->bits.ar, &k);
			if(!is_const(k.type))
				return 0;
		}

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

static decl_init *decl_init_brace_up(init_iter *, type_ref *);

static decl_init *decl_init_brace_up_array(
		init_iter *iter, type_ref *const tfor_wrapped)
{
	decl_init *first_init = iter->pos[0];

	/* '{' expected, if there isn't one, pretend there is */
	if(first_init->type == decl_init_scalar){
		decl_init *const ret = decl_init_new(decl_init_brace);
		type_ref *const next_type = type_ref_next(tfor_wrapped);

		do{
			dynarray_add(&ret->bits.inits,
					decl_init_brace_up(iter, next_type));

			first_init = iter->pos[0];
		}while(first_init);

		return ret;

	}else{
		return first_init;
	}
}

static decl_init *decl_init_brace_up_sue(
		init_iter *iter, struct_union_enum_st *sue)
{
	decl_init *first_init = iter->pos[0];

	if(first_init->type == decl_init_scalar){
		decl_init *const ret = decl_init_new(decl_init_brace);
		sue_member **mem_iter;

		for(mem_iter = sue->members; *mem_iter; mem_iter++){
			decl *smem = (*mem_iter)->struct_member;

			dynarray_add(&ret->bits.inits,
					decl_init_brace_up(iter, smem->ref));
		}

		return ret;
	}else{
		return first_init;
	}
}

static decl_init *decl_init_brace_up_scalar(
		init_iter *iter, type_ref *const tfor)
{
	decl_init *first_init = iter->pos[0];

	if(first_init->type == decl_init_brace){
		decl_init *inits[2] = {
			first_init, NULL
		};
		init_iter next = { inits };

		return decl_init_brace_up(&next, tfor);
	}

	return first_init;
}

static decl_init *decl_init_brace_up(init_iter *dinit, type_ref *tfor)
{
	struct_union_enum_st *sue;

	if((sue = type_ref_is_s_or_u(tfor)))
		return decl_init_brace_up_sue(dinit, sue);

	if(type_ref_is(tfor, type_ref_array))
		return decl_init_brace_up_array(dinit, tfor);

	return decl_init_brace_up_scalar(dinit, tfor);
}

static void DEBUG(decl_init *init, type_ref *tfor)
{
	decl_init *inits[2] = {
		init, NULL
	};
	init_iter next = { inits };

	decl_init *braced = decl_init_brace_up(&next, tfor);

	cc1_out = stdout;
	printf("braced = %p\n",braced);
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
