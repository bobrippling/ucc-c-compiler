#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include <stdlib.h>

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

static void decl_init_create_assignments_discard(
		decl_init *dinit,
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
			expr *e = dinit->bits.expr;
			intval iv;
			enum constyness type;

			FOLD_EXPR(e, stab);
			const_fold(e, &iv, &type);

			dinit->bits.expr = e;

			return type != CONST_NO;
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

type_ref *decl_initialise_array(decl_init *dinit, type_ref *tfor, expr *base, stmt *init_code)
{
	type_ref *tfor_deref;
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

			DIE_AT(&dinit->where, "array must be initalised with initialiser list");
		case decl_init_brace:
			break;
	}

	tfor_deref = type_ref_is(tfor, type_ref_array)->ref;

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

	{
		decl_init **it = dinit ? dinit->bits.inits : NULL;
		decl_init *chosen;
		const int n = dynarray_count((void **)it);
		int i;

		for(i = 0; i < n;){
			int free_chosen = 0;
			int this_count;

			/* index into the main-array */
			expr *this;
			/* `base`[i] */ {
				expr *op = expr_new_op(op_plus);
				op->lhs = base;
				op->rhs = expr_new_val(i);
				this = expr_new_deref(op);
			}

			if(it){
				if(it[i]->type == decl_init_brace){
					/* just pass this in - sub-initalised fully */
					chosen = it[i];
					this_count = 1;
				}else{
					/* pull as many _scalars_ as we need */
					int max, j;

					if(type_ref_is(tfor_deref, type_ref_array)){
						max = type_ref_array_len(tfor_deref);
					}else{
						/* assume scalar for now */
						max = 1;
					}

					free_chosen = 1;
					chosen = decl_init_new(decl_init_brace);
					chosen->bits.inits = umalloc((max + 1) * sizeof *chosen->bits.inits);

					for(j = 0; j < max; j++)
						chosen->bits.inits[j + i] = it[i];

					while(j < max){
						decl_init *tim;
						tim = chosen->bits.inits[j + i] = decl_init_new(decl_init_scalar);
						tim->bits.expr = expr_new_val(0);
					}

					this_count = max;
				}
			}else{
				/* fill with zero */
				chosen = NULL;
				this_count = 1;
			}

			decl_init_create_assignments_discard(chosen, tfor_deref, this, init_code);

			complete_to++;
			i += this_count;

			if(free_chosen){
				decl_init_free_1(chosen);
			}
		}
	}

	/* patch the type size */
	if(type_ref_is_incomplete_array(tfor))
		tfor = type_ref_complete_array(tfor, complete_to);

	return tfor;
}

static void decl_initialise_sue(decl_init *dinit,
		struct_union_enum_st *sue, expr *base, stmt *init_code)
{
	/* iterate over each member, pulling from the dinit */
	sue_member **smem;
	decl_init **p_subinit;

	if(dinit == NULL)
		ICE("TODO: null dinit for struct");

	if(dinit->type != decl_init_brace)
		DIE_AT(&dinit->where, "%s must be initalised with initialiser list",
				sue_str(sue));

	for(p_subinit = dinit->bits.inits, smem = sue->members;
			smem && *smem;
			(*p_subinit ? ++p_subinit : 0), smem++)
	{
		decl *const sue_mem = (*smem)->struct_member;

		/* room for optimisation below - avoid sue name lookup */
		expr *accessor = expr_new_struct(base, 1 /* a.b */,
				expr_new_identifier(sue_mem->spel));

		if(*p_subinit){
			decl_init_create_assignments_discard(*p_subinit,
					sue_mem->ref,
					accessor,
					init_code);

		}else{
			ICE("TODO: uninitialised struct tailing members");
		}
	}
}

static type_ref *decl_init_create_assignments(
		decl_init *dinit,
		type_ref *const tfor_wrapped, /* could be typedef/cast */
		expr *base,
		stmt *init_code)
{
	/* iterate over tfor's array/struct members/scalar,
	 * pulling from dinit as necessary */
	type_ref *tfor, *tfor_ret = tfor_wrapped;
	struct_union_enum_st *sue;

	if((tfor = type_ref_is(tfor_wrapped, type_ref_array))){
		tfor_ret = decl_initialise_array(dinit, tfor, base, init_code);

	}else if((sue = type_ref_is_s_or_u(tfor_wrapped))){
		decl_initialise_sue(dinit, sue, base, init_code);

	}else{
		expr *assign_from, *assign_init;

		if(dinit){
			while(dinit && dinit->type == decl_init_brace){
				WARN_AT(&dinit->where, "excess braces around scalar initialiser");

				if(dinit->bits.inits){
					dinit = dinit->bits.inits[0];
				}else{
					WARN_AT(&dinit->where, "empty initaliser");
					goto zero_init;
				}
			}

			if(!dinit)
				goto zero_init;

			assert(dinit->type == decl_init_scalar);
			assign_from = dinit->bits.expr;
		}else{
zero_init:
			assign_from = expr_new_val(0);
		}

		assign_init = expr_new_assign(base, assign_from);
		assign_init->assign_is_init = 1;

		dynarray_add((void ***)&init_code->codes,
				expr_to_stmt(assign_init, init_code->symtab));
	}

	return tfor_ret;
}

static void decl_init_create_assignments_discard(
		decl_init *dinit, type_ref *const tfor_wrapped,
		expr *base, stmt *init_code)
{
	type_ref *t = decl_init_create_assignments(dinit, tfor_wrapped, base, init_code);

	if(t != tfor_wrapped)
		type_ref_free_1(t);
}

void decl_init_create_assignments_for_spel(decl *d, stmt *init_code)
{
	d->ref = decl_init_create_assignments(
			d->init, d->ref,
			expr_new_identifier(d->spel), init_code);
}

void decl_init_create_assignments_for_base(decl *d, expr *base, stmt *init_code)
{
	d->ref = decl_init_create_assignments(
			d->init, d->ref, base, init_code);
}
