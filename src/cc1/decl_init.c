#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "cc1.h"
#include "fold.h"
#include "const.h"
#include "macros.h"

#include "decl_init.h"

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
			int k = 1;

			for(i = dinit->bits.inits; i && *i; i++){
				int sub_k = decl_init_is_const(*i, stab);

				if(!sub_k)
					k = 0;
			}

			return k;
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

const char *decl_init_to_str(enum decl_init_type t)
{
	switch(t){
		CASE_STR_PREFIX(decl_init, scalar);
		CASE_STR_PREFIX(decl_init, brace);
	}
	return NULL;
}

void decl_init_complete_array(type_ref **ptfor, decl_init *init_from)
{
	(void)ptfor;
	(void)init_from;
	ICE("TODO");
}

void decl_initialise_array(decl_init *dinit, type_ref *tfor, expr *base, stmt *init_code)
{
	type_ref *const tfor_deref = type_ref_is(tfor, type_ref_array)->ref;
	decl_init **it;
	int i;

	for(i = 0, it = dinit->bits.inits; it && *it; it++, i++){
		expr *this;
		/* `base`[i] */ {
			expr *op = expr_new_op(op_plus);
			op->lhs = base;
			op->rhs = expr_new_val(i);
			this = expr_new_deref(op);
		}

		decl_init_create_assignments(*it, tfor_deref, this, init_code);
	}
}

void decl_init_create_assignments(
		decl_init *dinit,
		type_ref *const tfor_wrapped, /* could be typedef/cast */
		expr *base,
		stmt *init_code)
{
	/* iterate over tfor's array/struct members/scalar,
	 * pulling from dinit as necessary */
	type_ref *tfor;
	struct_union_enum_st *sue;

	if((tfor = type_ref_is(tfor_wrapped, type_ref_array))){
		switch(dinit->type){
			case decl_init_scalar:
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

		decl_initialise_array(dinit, tfor, base, init_code);

	}else if((sue = type_ref_is_s_or_u(tfor_wrapped))){
		ICE("TODO: sue init");

	}else{
		expr *assign_from, *assign_init;

		if(dinit){
			while(dinit->type == decl_init_brace){
				WARN_AT(&dinit->where, "excess braces around scalar initialiser");

				if(dinit->bits.inits){
					dinit = dinit->bits.inits[0];
				}else{
					WARN_AT(&dinit->where, "empty initaliser");
					goto zero_init;
				}
			}

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
}

void decl_init_create_assignments_for_spel(decl *d, stmt *init_code)
{
	/* special case of incomplete-array */
	if(d->init->type == decl_init_brace
	&& type_ref_is_incomplete_array(d->ref))
	{
		int n = dynarray_count((void **)d->init->bits.inits);
		d->ref = type_ref_complete_array(d->ref, n); /* XXX: memleak */
	}

	decl_init_create_assignments(
			d->init, d->ref,
			expr_new_identifier(d->spel), init_code);
}
