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

void decl_init_create_assignments_for_spel(decl *d, stmt *init_code)
{
	decl_init_create_assignments(
			d->init, d->ref,
			expr_new_identifier(d->spel), init_code);
}

void fold_decl_init(decl_init *di, symtable *stab)
{
	/* only fold scalars
	 * brace-inits are done on the next pass by the caller */
	if(di->type == decl_init_scalar)
		FOLD_EXPR(di->bits.expr, stab);
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

	fold_decl_init(dinit, init_code->symtab);

	if((tfor = type_ref_is(tfor_wrapped, type_ref_array))){
		switch(dinit->type){
			case decl_init_scalar:
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

		ICE("TODO: array init");

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

#if 0
f(){
	if(type_ref_is(tfor, type_ref_array)){
		type_ref *tfor_deref;
		int array_limit;
		int current_count, wanted_count, array_size;

		if(init_from->type == decl_init_scalar)
			DIE_AT(&init_from->where, "arrays must be initialised with an initialiser list");

		tfor_deref = tfor->ref;

		if((tfor_deref = type_ref_is(tfor_deref, type_ref_array))){
			/* int [2] - (2 * 4) / 4 = 2 */
			wanted_count = type_ref_array_len(tfor_deref);
		}else{
			wanted_count = 1;
		}

		array_limit = type_ref_is(tfor, type_ref_array) ? type_ref_array_len(tfor) : -1;
		array_size = 0;

		{
			decl_init **i;
			int i_index;

			current_count = 0;

			/* count */
			for(i = init_from->bits.inits, i_index = 0; i && *i; i++, i_index++){
				decl_init *sub = *i;

				/* access the i'th element of base */
				expr *target = expr_new_op(op_plus);

				target->lhs = base;
				target->rhs = expr_new_val(i_index);

				target = expr_new_deref(target);

				fold_gen_init_assignment2(target, tfor_deref, sub, codes);

				if(sub->type == decl_init_scalar){
					/* int x[][2] = { 1, 2, ... }
					 *                   ^ increment current count,
					 *                     moving onto the next if needed
					 */
					current_count++;

					if(current_count == wanted_count){
						/* move onto next */
						goto next_ar;
					}
				}else{
					/* sub is an {...} */
next_ar:
					array_size++;
					current_count = 0;
					if(array_size == array_limit){
						WARN_AT(&sub->where, "excess initialiser");
						break;
					}
				}
			}
		}

		ICE("TODO: decl array init");
	}else{
		/* scalar init */
		switch(init_from ? init_from->type : decl_init_scalar){
			case decl_init_scalar:
			{
				expr *assign_init;

				assign_init = expr_new_assign(
						base,
						init_from ? init_from->bits.expr : expr_new_val(0)
						/* 0 = default value for initialising */
						);

				assign_init->assign_is_init = 1;

				dynarray_add((void ***)&codes->inits,
						expr_to_stmt(assign_init, codes->symtab));
				break;
			}

			case decl_init_brace:
				/*if(n_inits > 1)
					WARN_AT(&init_from->where, "excess initialisers for scalar");*/

				fold_gen_init_assignment2(base, tfor,
						init_from->bits.inits ? init_from->bits.inits[0] : NULL,
						codes);
				break;
		}
	}
}
#endif
