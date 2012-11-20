#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "fold.h"
#include "fold_sym.h"
#include "sym.h"
#include "../util/platform.h"
#include "const.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/dynmap.h"
#include "sue.h"
#include "decl.h"
#include "decl_init.h"
#include "pack.h"
#include "funcargs.h"

decl     *curdecl_func;
type_ref *curdecl_ref_func_called; /* for funcargs-local labels and return type-checking */

static where asm_struct_enum_where;

void fold_type_ref_equal(
		type_ref *a, type_ref *b, where *w, enum warning warn,
		const char *errfmt, ...)
{
	enum decl_cmp flags = DECL_CMP_ALLOW_VOID_PTR;

	/* stronger checks for blocks */
	if(type_ref_is(a, type_ref_block) || type_ref_is(b, type_ref_block))
		flags |= DECL_CMP_EXACT_MATCH;

	if(!type_ref_equal(a, b, flags)){
		int one_struct;
		va_list l;

		/*cc1_warn_at(w, 0, 0, warn, "%s vs. %s for...", decl_to_str(a), decl_to_str_r(buf, b));*/


		one_struct = type_ref_is_s_or_u(a) || type_ref_is_s_or_u(b);

		va_start(l, errfmt);
		cc1_warn_atv(w, one_struct || type_ref_is_void(a) || type_ref_is_void(b), 1, warn, errfmt, l);
		va_end(l);
	}
}

void fold_insert_casts(type_ref *dlhs, expr **prhs, symtable *stab, where *w, const char *desc)
{
	expr *const rhs = *prhs;

	if(!type_ref_equal(dlhs, rhs->tree_type,
				DECL_CMP_ALLOW_VOID_PTR |
				DECL_CMP_EXACT_MATCH))
	{
		/* insert a cast: rhs -> lhs */
		expr *cast;

		cast = expr_new_cast(dlhs, 1);
		cast->expr = rhs;
		*prhs = cast;

		/* need to fold the cast again - mainly for "loss of precision" warning */
		fold_expr_cast_descend(cast, stab, 0);
	}

	if(type_ref_is_signed(dlhs) != type_ref_is_signed(rhs->tree_type)){
		enum constyness type;
		intval iv;

		const_fold(rhs, &iv, &type);

#define SPEL_IF_IDENT(hs)                          \
		expr_kind(hs, identifier) ? " ("     : "", \
		expr_kind(hs, identifier) ? hs->spel : "", \
		expr_kind(hs, identifier) ? ")"      : ""

		cc1_warn_at(w, 0, 1, WARN_SIGN_COMPARE,
				"operation between signed and unsigned%s%s%s in %s",
				SPEL_IF_IDENT(rhs), desc);
	}
}


void fold_check_restrict(expr *lhs, expr *rhs, const char *desc, where const *w)
{
	/* restrict operation checks */
	const enum type_qualifier ql = type_ref_qual(lhs->tree_type),
				                    qr = type_ref_qual(rhs->tree_type);

	if((ql & qual_restrict) && (qr & qual_restrict))
		WARN_AT(w, "restrict pointers in %s", desc);
}

int fold_get_sym(expr *e, symtable *stab)
{
	if(e->sym)
		return 1;

	if(e->spel)
		return !!(e->sym = symtab_search(stab, e->spel));

	return 0;
}

void fold_inc_writes_if_sym(expr *e, symtable *stab)
{
	if(fold_get_sym(e, stab))
		e->sym->nwrites++;
}

expr *fold_expr(expr *e, symtable *stab)
{
	if(e->tree_type)
		goto fin;

	fold_get_sym(e, stab);

	EOF_WHERE(&e->where, e->f_fold(e, stab));

	UCC_ASSERT(e->tree_type, "no tree_type after fold (%s)", e->f_str());

	/* perform array decay and pointer decay */
	{
		type_ref *r = e->tree_type;
		expr *imp_cast = NULL;

		EOF_WHERE(&e->where,
				type_ref *decayed = type_ref_decay(r);

				if(decayed != r)
					imp_cast = expr_new_cast(decayed, 1);
			);

		if(imp_cast){
			imp_cast->expr = e;
			fold_expr_cast_descend(imp_cast, stab, 0);
			e = imp_cast;
		}
	}

fin:
	return e;
}

void fold_enum(struct_union_enum_st *en, symtable *stab)
{
	const int bitmask = decl_attr_present(en->attr, attr_enum_bitmask);
	sue_member **i;
	int defval = bitmask;

	for(i = en->members; *i; i++){
		enum_member *m = (*i)->enum_member;
		expr *e = m->val;

		/* -1 because we can't do dynarray_add(..., 0) */
		if(e == (expr *)-1){

			/*expr_free(e); XXX: memleak */
			EOF_WHERE(&asm_struct_enum_where,
				m->val = expr_new_val(defval)
			);

			if(bitmask)
				defval <<= 1;
			else
				defval++;

		}else{
			intval iv;

			FOLD_EXPR(e, stab);
			const_fold_need_val(e, &iv);
			m->val = e;

			defval = bitmask ? iv.val << 1 : iv.val + 1;
		}
	}
}

void fold_sue(struct_union_enum_st *const sue, symtable *stab, int *poffset, int *pthis)
{
	if(sue->primitive == type_enum){
		int sz;
		fold_enum(sue, stab);
		sz = sue_size(sue, &sue->where);
		pack_next(poffset, pthis, sz, sz);
	}else{
		int align_max = 1;
		sue_member **i;

		for(i = sue->members; i && *i; i++){
			decl *d = (*i)->struct_member;
			int align;
			struct_union_enum_st *this_sue;

			fold_decl(d, stab);

			if((this_sue = type_ref_is_s_or_u(d->ref)) && !type_ref_is(d->ref, type_ref_ptr)){
				if(this_sue == sue)
					DIE_AT(&d->where, "nested %s", sue_str(sue));

				fold_sue(this_sue, stab, poffset, pthis);

				align = this_sue->align;
			}else{
				const int sz = decl_size(d, &d->where);
				pack_next(poffset, pthis, sz, sz);
				align = sz; /* for now */
			}


			if(sue->primitive == type_struct)
				d->struct_offset = *pthis;
			/* else - union, all offsets are the same */

			if(align > align_max)
				align_max = align;
		}

		sue->align = align_max;
	}
}

void fold_complete_array(type_ref **ptfor, decl_init *init_from)
{
	const int n_inits = init_from ? decl_init_len(init_from) : 0;
	type_ref *tfor = *ptfor;

	/* TODO: struct check also */
	if(init_from && DECL_IS_ARRAY(tfor) && init_from->type != decl_init_brace)
		DIE_AT(&init_from->where, "array must be initialised with an initialiser list");

	if(type_ref_is_incomplete_array(tfor)){
		/* case 1: int x[][2] = { 0, 1, 2, 3 }
		 * case 2: int x[][2] = { {1}, {2} }
		 * case 3: int x[][2] = { {1}, 2, 3 }
		 * case 4: int x[][2] = { 1, 2, {3} }
		 */
		int complete_to;

		if(!init_from)
			DIE_AT(&tfor->where, "can't complete array - no initialiser");

		/* decide based on the first sub */
		switch(init_from->bits.inits[0]->type){
			default:
				ICE("invalid initialiser");

			case decl_init_scalar:
			{
				/*
				 * this must work for:
				 *
				 * int x[][2] = {   1, 2,    { 3, 4 } };
				 * int x[][2] = {   1, 2,      3, 4   };
				 * int x[][2] = { { 1, 2, }, { 3, 4 } };
				 * int x[][2] = {   1,       { 3, 4 } };
				 */

				/* FIXME ->ref */
				type_ref *t_derefed = type_ref_ptr_depth_dec(tfor->ref);

				/* check for different sub-inits */
#if 0
				for(){
				}
#else
				ICE("TODO");
#endif

				/* (int[2] size = 8) / type_size = 2 */
				complete_to = n_inits / (type_ref_size(t_derefed, &t_derefed->where) / type_ref_size(t_derefed->ref, &t_derefed->where));

#ifdef DECL_COMP_VERBOSE
				fprintf(stderr, "n_inits = %d, decl_size(*(%s)) = %d, type_size(%s) = %d\n",
						n_inits, decl_to_str(tfor), decl_size(t_derefed),
						type_to_str(t_derefed->type), type_size(t_derefed->type));

				fprintf(stderr, "completing array (subtype %s) to %d\n",
						decl_to_str(dtmp), complete_to);
#endif
				break;
			}

			case decl_init_brace:
				complete_to = n_inits;
				break;
		}

		*ptfor = tfor = type_ref_complete_array(tfor, complete_to);
	}
}

void fold_gen_init_assignment2(
		expr *base, type_ref *tfor,
		decl_init *init_from, stmt *codes)
{
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

void fold_gen_init_assignment_base(expr *base, decl *dfor, stmt *code)
{
	fold_gen_init_assignment2(base, dfor->ref, dfor->init, code);
}

void fold_gen_init_assignment(decl *dfor, stmt *code)
{
	fold_gen_init_assignment_base(expr_new_identifier(dfor->spel), dfor, code);
}

#if 0
/* see fold_gen_init_assignment2 */
void fold_decl_init(decl *for_decl, decl_init *di, symtable *stab)
{
	fold_gen_init_assignment(for_decl, codes);

	/* fold + type check for statics + globals */
	if(DECL_IS_ARRAY(for_decl)){
		/* don't allow scalar inits */
		switch(di->type){
			case decl_init_struct:
			case decl_init_scalar:
				DIE_AT(&for_decl->where, "can't initialise array decl with scalar or struct/union");

			case decl_init_brace:
			{
				decl_desc *dp = decl_desc_tail(for_decl);
				intval sz;

				UCC_ASSERT(dp->type == decl_desc_array, "not array");

				const_fold_need_val(dp->bits.array_size, &sz);

				if(sz.val == 0){
					/* complete the decl */
					expr *expr_sz = dp->bits.array_size;
					expr_mutate_wrapper(expr_sz, val);
					expr_sz->val.iv.val = decl_init_len(di);
				}
			}
		}
	}else if(for_decl->type->primitive == type_struct && !decl_is_ptr(for_decl)){
		/* similar to above */
		const int nmembers = sue_nmembers(for_decl->type->sue);

		switch(di->type){
			case decl_init_scalar:
				/*ICE("TODO: %s init with scalar", "struct/union");*/
				DIE_AT(&di->where, "can't initialise %s with expression",
						decl_to_str(for_decl));

			case decl_init_struct:
			{
				ICE("TODO");
#if 0
				const int ninits = dynarray_count((void **)di->bits.subs);
				int i;

				/* for_decl is a struct - the above else-if */

				for(i = 0; i < ninits; i++){
					struct decl_init_sub *sub = di->bits.subs[i];
					sub->for_decl = struct_union_member_find(for_decl->type->sue, sub->spel, &for_decl->where);
				}
#endif

				fprintf(stderr, "checked decl struct/union init for %s\n", for_decl->spel);
				break;
			}

			case decl_init_brace:
				if(for_decl->type->sue){
					/* init struct with braces */
					const int nbraces  = dynarray_count((void **)di->bits.subs);

					if(nbraces > nmembers)
						DIE_AT(&for_decl->where, "too many initialisers for %s", decl_to_str(for_decl));

					/* folded below */

				}else{
					DIE_AT(&for_decl->where, "initialisation of incomplete %s", sue_str(...));
				}
				break;
		}
	}
#endif

#if 0
	switch(di->type){
		case decl_init_brace:
		case decl_init_struct:
		{
			struct_union_enum_st *const sue = for_decl->type->sue;
			decl_init_sub *s;
			int i;
			int struct_index = 0;

			/* recursively fold */
			for(i = 0; (s = di->bits.subs[i]); i++, struct_index++){
				decl *new_for_decl;

				if(s->spel){
					decl *member;
					int old_idx;

					if(di->type != decl_init_struct)
						DIE_AT(&di->where, "can't initialise array with struct-style init");

					ICE(".x struct/union init");

					member = struct_union_member_find(sue, s->spel, &di->where);

					old_idx = struct_index;
					struct_index = struct_union_member_idx(sue, member);

					/* update where we are in the struct - FIXME: duplicate checks */
					if(struct_index <= old_idx){
						DIE_AT(&di->where, "duplicate initialisation of %s %s member %s",
								decl_to_str(for_decl), sue->spel, member->spel);
					}
				}

				if(decl_is_struct_or_union(for_decl)){
					/* take the struct member type */
					new_for_decl = struct_union_member_at_idx(sue, struct_index);

					/* this should be caught above */
					if(!new_for_decl)
						DIE_AT(&s->where, "excess element for %s initialisation", decl_to_str(for_decl));

				}else{
					/* decl from the array type */
					new_for_decl = decl_ptr_depth_dec(decl_copy(for_decl), &for_decl->where);
				}

				fold_decl_init(new_for_decl, s->init, stab);
			}

			if(decl_is_struct_or_union(for_decl)){
				const int nmembers = sue_nmembers(sue);

				/* add to the end - since .x = y isn't available, don't have to worry about middle ones */
				for(; i < nmembers; i++){
					decl_init_sub *sub = decl_init_sub_zero_for_decl(struct_union_member_at_idx(sue, i));
					dynarray_add((void ***)&di->bits.subs, sub);
				}
			}
		}
		break;
	}
}
#endif

void fold_type_ref(type_ref *r, type_ref *parent, symtable *stab)
{
	enum type_qualifier q_to_check = qual_none;

	if(!r)
		return;

	switch(r->type){
	/* check for array of funcs, func returning array */
		case type_ref_array:
			if(type_ref_is(r->ref, type_ref_func))
				DIE_AT(&r->where, "array of functions");
			break;

		case type_ref_func:
			if(type_ref_is(r->ref, type_ref_func))
				DIE_AT(&r->where, "function returning a function");

			if(type_ref_is(parent, type_ref_ptr) && (type_ref_qual(parent) & qual_restrict))
				DIE_AT(&r->where, "restrict qualified function pointer");
			break;

		case type_ref_block:
			if(!type_ref_is(r->ref, type_ref_func))
				DIE_AT(&r->where, "invalid block pointer - function required (got %s)",
						type_ref_to_str(r->ref));

			/*q_to_check = r->bits.block.qual; - allowed */
			break;

		case type_ref_cast:
			q_to_check = type_ref_qual(r);
			break;

		case type_ref_ptr:
			/*q_to_check = r->bits.qual; - allowed */
			break;

		case type_ref_type:
			q_to_check = r->bits.type->qual;
			break;

		case type_ref_tdef:
		{
			expr **p_expr = &r->bits.tdef.type_of;

			/* q_to_check = TODO */
			FOLD_EXPR(*p_expr, stab);

			if(r->bits.tdef.decl)
				fold_decl(r->bits.tdef.decl, stab);

			break;
		}
	}

	if(q_to_check & qual_restrict)
		WARN_AT(&r->where, "restrict on non-pointer type '%s'", type_ref_to_str(r));

	fold_type_ref(r->ref, r, stab);
}

void fold_decl(decl *d, symtable *stab)
{
	/*
	 * now we've folded, check for restrict
	 * since typedef int *intptr; intptr restrict a; is valid
	 */
	fold_type_ref(d->ref, NULL, stab);

#if 0
	/* if we have a type and it's incomplete, error */
	no - only on use
	if(!type_ref_is_complete(d->ref))
		DIE_AT(&d->where, "use of incomplete type - %s (%s)", d->spel, decl_to_str(d));
#endif

#ifdef FIELD_WIDTH_TODO
	if(d->field_width){
		enum constyness ktype;
		intval iv;
		int width;
		type *t = ;

		FOLD_EXPR(d->field_width, stab);
		const_fold(d->field_width, &iv, &ktype);

		width = iv.val;

		if(ktype != CONST_WITH_VAL)
			DIE_AT(&d->where, "constant expression required for field width");

		if(width <= 0)
			DIE_AT(&d->where, "field width must be positive");

		if(!decl_is_integral(d))
			DIE_AT(&d->where, "field width on non-integral type %s", decl_to_str(d));

		if(width == 1 && t->is_signed)
			WARN_AT(&d->where, "%s 1-bit field width is signed (-1 and 0)", decl_to_str(d));
	}
#endif

	/* allow:
	 *   register int (*f)();
	 * disallow:
	 *   register int   f();
	 *   register int  *f();
	 */
	if(DECL_IS_FUNC(d)){
		switch(d->store){
			case store_register:
			case store_auto:
				DIE_AT(&d->where, "%s storage for function", decl_store_to_str(d->store));
			default:
				break;
		}

		if(!d->func_code){
			/* prototype - set extern, so we get a symbol generated (if needed) */
			switch(d->store){
				case store_default:
					d->store = store_extern;
				case store_extern:
				default:
					break;
			}
		}
	}else if(d->is_inline){
		WARN_AT(&d->where, "inline on non-function");
	}

	if(d->init){
		/* should the store be on the type? */
		if(d->store == store_extern){
			/* allow for globals - remove extern since it's a definition */
			if(stab->parent){
				DIE_AT(&d->where, "externs can't be initialised");
			}else{
				WARN_AT(&d->where, "extern initialisation");
				d->store = store_default;
			}
		}
	}
}

void fold_decl_global_init(decl *d, symtable *stab)
{
	int k;

	if(!d->init)
		return;

	k = decl_init_is_const(d->init, stab);

	fold_complete_array(&d->ref, d->init);

	if(!k){
		DIE_AT(&d->init->where, "%s initialiser not constant (%s)",
				stab->parent ? "static" : "global", decl_init_to_str(d->init->type));
	}
}

void fold_decl_global(decl *d, symtable *stab)
{
	switch(d->store){
		case store_extern:
		case store_default:
		case store_static:
		case store_inline:
			break;

		case store_typedef:
			ICE("typedef store");

		case store_auto:
		case store_register:
			DIE_AT(&d->where, "invalid storage class %s on global scoped %s",
					decl_store_to_str(d->store),
					DECL_IS_FUNC(d) ? "function" : "variable");
	}

	fold_decl(d, stab);

	/*
	 * inits are normally handled in stmt_code,
	 * but this is global, handle here
	 */
	fold_decl_global_init(d, stab);
}

void fold_symtab_scope(symtable *stab)
{
	struct_union_enum_st **sit;

	for(sit = stab->sues; sit && *sit; sit++){
		int this, offset = 0;
		fold_sue(*sit, stab, &offset, &this);
	}
}

void fold_need_expr(expr *e, const char *stmt_desc, int is_test)
{
	if(type_ref_is_void(e->tree_type))
		DIE_AT(&e->where, "%s requires non-void expression", stmt_desc);

	if(!e->in_parens && expr_kind(e, assign))
		cc1_warn_at(&e->where, 0, 1, WARN_TEST_ASSIGN, "testing an assignment in %s", stmt_desc);

	if(is_test){
		if(!type_ref_is_bool(e->tree_type)){
			cc1_warn_at(&e->where, 0, 1, WARN_TEST_BOOL, "testing a non-boolean expression, %s, in %s",
					type_ref_to_str(e->tree_type), stmt_desc);
		}

		if(expr_kind(e, addr)){
			cc1_warn_at(&e->where, 0, 1, WARN_TEST_BOOL/*FIXME*/,
					"testing an address is always true");
		}
	}

	fold_disallow_st_un(e, stmt_desc);
}

void fold_disallow_st_un(expr *e, const char *desc)
{
	struct_union_enum_st *sue;

	if((sue = type_ref_is_s_or_u(e->tree_type))){
		DIE_AT(&e->where, "%s involved in %s",
				sue_str(sue), desc);
	}
}

#ifdef SYMTAB_DEBUG
void print_stab(symtable *st, int current, where *w)
{
	decl **i;

	if(st->parent)
		print_stab(st->parent, 0, NULL);

	if(current)
		fprintf(stderr, "[34m");

	fprintf(stderr, "\ttable %p, children %d, vars %d, parent: %p",
			(void *)st,
			dynarray_count((void **)st->children),
			dynarray_count((void **)st->decls),
			(void *)st->parent);

	if(current)
		fprintf(stderr, "[m%s%s", w ? " at " : "", w ? where_str(w) : "");

	fputc('\n', stderr);

	for(i = st->decls; i && *i; i++)
		fprintf(stderr, "\t\tdecl %s\n", (*i)->spel);
}
#endif

void fold_stmt(stmt *t)
{
	UCC_ASSERT(t->symtab->parent, "symtab has no parent");

#ifdef SYMTAB_DEBUG
	if(stmt_kind(t, code)){
		fprintf(stderr, "fold-code, symtab:\n");
		PRINT_STAB(t, 1);
	}
#endif

	t->f_fold(t);
}

void fold_stmt_and_add_to_curswitch(stmt *t)
{
	fold_stmt(t->lhs); /* compound */

	if(!t->parent)
		DIE_AT(&t->where, "%s not inside switch", t->f_str());

	dynarray_add((void ***)&t->parent->codes, t);

	/* we are compound, copy some attributes */
	t->kills_below_code = t->lhs->kills_below_code;
	/* TODO: copy ->freestanding? */
}

void fold_funcargs(funcargs *fargs, symtable *stab, char *context)
{
	if(fargs->arglist){
		/* check for unnamed params and extern/static specs */
		int i;

		for(i = 0; fargs->arglist[i]; i++){
			decl *const d = fargs->arglist[i];

			/* convert any array definitions and functions to pointers */
			EOF_WHERE(&d->where,
				decl_conv_array_func_to_ptr(d) /* must be before the decl is folded (since fold checks this) */
			);

			fold_decl(d, stab);

			if(decl_store_static_or_extern(d->store)){
				const char *sp = d->spel;
				DIE_AT(&fargs->where, "argument %d %s%s%sin function \"%s\" is static or extern",
						i + 1,
						sp ? "(" : "",
						sp ? sp  : "",
						sp ? ") " : "",
						context);
			}
		}
	}
}

int fold_passable_yes(stmt *s)
{ (void)s; return 1; }

int fold_passable_no(stmt *s)
{ (void)s; return 0; }

int fold_passable(stmt *s)
{
	return s->f_passable(s);
}

void fold_func(decl *func_decl)
{
	if(func_decl->func_code){
		struct
		{
			char *extra;
			where *where;
		} the_return = { NULL, NULL };

		curdecl_func = func_decl;
		curdecl_ref_func_called = type_ref_func_call(curdecl_func->ref, NULL);

		UCC_ASSERT(type_ref_is(func_decl->ref, type_ref_func), "not a func");
		symtab_add_args(
				func_decl->func_code->symtab,
				func_decl->ref->bits.func,
				func_decl->spel);

		fold_stmt(func_decl->func_code);

		if(decl_has_attr(curdecl_func, attr_noreturn)){
			if(!type_ref_is_void(curdecl_ref_func_called)){
				cc1_warn_at(&func_decl->where, 0, 1, WARN_RETURN_UNDEF,
						"function \"%s\" marked no-return has a non-void return value",
						func_decl->spel);
			}


			if(fold_passable(func_decl->func_code)){
				/* if we reach the end, it's bad */
				the_return.extra = "implicitly ";
				the_return.where = &func_decl->where;
			}else{
				stmt *ret = NULL;

				stmt_walk(func_decl->func_code, stmt_walk_first_return, NULL, &ret);

				if(ret){
					/* obviously returns */
					the_return.extra = "";
					the_return.where = &ret->where;
				}
			}

			if(the_return.extra){
				cc1_warn_at(the_return.where, 0, 1, WARN_RETURN_UNDEF,
						"function \"%s\" marked no-return %sreturns",
						func_decl->spel, the_return.extra);
			}

		}else if(!type_ref_is_void(curdecl_ref_func_called)){
			/* non-void func - check it doesn't return */
			if(fold_passable(func_decl->func_code)){
				cc1_warn_at(&func_decl->where, 0, 1, WARN_RETURN_UNDEF,
						"control reaches end of non-void function %s",
						func_decl->spel);
			}
		}

		curdecl_ref_func_called = NULL;
		curdecl_func = NULL;
	}
}

static void fold_link_decl_defs(dynmap *spel_decls)
{
	int i;

	for(i = 0; ; i++){
		char *key;
		char wbuf[WHERE_BUF_SIZ];
		decl *d, *e, *definition, *first_none_extern;
		decl **decls_for_this, **decl_iter;
		int count_inline, count_extern, count_static, count_total;

		key = dynmap_key(spel_decls, i);
		if(!key)
			break;

		decls_for_this = dynmap_get(spel_decls, key);
		d = *decls_for_this;

		definition = decl_is_definition(d) ? d : NULL;

		count_inline = d->is_inline;
		count_extern = count_static = 0;
		first_none_extern = NULL;

		switch(d->store){
			case store_extern:
				count_extern++;
				break;

			case store_static:
				count_static++;
				/* fall */
			default:
				first_none_extern = d;
				break;
		}

		/*
		 * check the first is equal to all the rest, strict-types
		 * check they all have the same static/non-static storage
		 * if all are extern (and not initialised), the decl is extern
		 * if all are extern but there is an init, the decl is global
		 */

		for(decl_iter = decls_for_this + 1; (e = *decl_iter); decl_iter++){
			/* check they are the same decl */
			if(!decl_equal(d, e, DECL_CMP_EXACT_MATCH)){
				char buf[DECL_STATIC_BUFSIZ];

				DIE_AT(&e->where, "mismatching declaration of %s\n%s\n%s vs %s",
						d->spel,
						where_str_r(wbuf, &d->where),
						decl_to_str_r(buf, d),
						decl_to_str(       e));
			}

			if(decl_is_definition(e)){
				/* e is the implementation/instantiation */

				if(definition){
					/* already got one */
					DIE_AT(&e->where, "duplicate definition of %s (%s)", d->spel, where_str_r(wbuf, &d->where));
				}

				definition = e;
			}

			count_inline += e->is_inline;

			switch(e->store){
				case store_extern:
					count_extern++;
					break;

				case store_static:
					count_static++;
					/* fall */
				default:
					if(!first_none_extern)
						first_none_extern = e;
					break;
			}
		}

		if(!definition){
      /* implicit definition - attempt a not-extern def if we have one */
      if(first_none_extern)
        definition = first_none_extern;
      else
        definition = d;
		}

		count_total = dynarray_count((void **)decls_for_this);

		if(DECL_IS_FUNC(definition)){
			/*
			 * inline semantics
			 *
			 * all "inline", none "extern" = inline_only
			 * "static inline" = code emitted, decl is static
			 * one "inline", and "extern" mentioned, or "inline" not mentioned = code emitted, decl is extern
			 */
			definition->is_inline = count_inline > 0;


			/* all defs must be static, except the def, which is allowed to be non-static */
			if(count_static > 0){
				definition->store = store_static;

				if(count_static != count_total && (definition->func_code ? count_static != count_total - 1 : 0)){
					DIE_AT(&definition->where,
							"static/non-static mismatch of function %s (%d static defs vs %d total)",
							definition->spel, count_static, count_total);
				}
			}


			if(definition->store == store_static){
				/* static inline */

			}else if(count_inline == count_total && count_extern == 0){
				/* inline only */
				definition->inline_only = 1;
				WARN_AT(&definition->where, "definition is inline-only (ucc doesn't inline currently)");
			}else if(count_inline > 0 && (count_extern > 0 || count_inline < count_total)){
				/* extern inline */
				definition->store = store_extern;
			}

			if(definition->is_inline && !definition->func_code)
				WARN_AT(&definition->where, "inline function missing implementation");

		}else if(count_static && count_static != count_total){
			/* TODO: iter through decls, printing them out */
			DIE_AT(&definition->where, "static/non-static mismatch of %s", definition->spel);
		}

		definition->is_definition = 1;

		/*
		 * func -> extern (if no func code) done elsewhere,
		 * since we need to do it for local decls too
		 */
	}
}

void fold(symtable *globs)
{
#define D(x) globs->decls[x]
	extern const char *current_fname;
	dynmap *spel_decls;
	int i;

	memset(&asm_struct_enum_where, 0, sizeof asm_struct_enum_where);
	asm_struct_enum_where.fname = current_fname;

	if(fopt_mode & FOPT_ENABLE_ASM){
		decl *df;
		funcargs *fargs;
		const where *old_w;

		old_w = eof_where;
		eof_where = &asm_struct_enum_where;

		df = decl_new();
		df->spel = ustrdup(ASM_INLINE_FNAME);

		fargs = funcargs_new();
		fargs->arglist    = umalloc(2 * sizeof *fargs->arglist);
		fargs->arglist[1] = NULL;

		/* const char * */
		(fargs->arglist[0] = decl_new())->ref = type_ref_new_ptr(
				type_ref_new_type(type_new_primitive_qual(type_char, qual_const)),
				qual_none);

		df->ref = type_ref_new_func(type_ref_new_INT(), fargs);

		symtab_add(globs, df, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);

		eof_where = old_w;
	}

	fold_symtab_scope(globs);

	for(i = 0; D(i); i++)
		if(D(i)->sym)
			ICE("%s: sym (%p) already set for global \"%s\"", where_str(&D(i)->where), (void *)D(i)->sym, D(i)->spel);

	spel_decls = dynmap_new((dynmap_cmp_f *)strcmp);

	for(;;){
		int i;

		/* find the next sym (since we can prepend, start at 0 each time */
		for(i = 0; D(i); i++)
			if(!D(i)->sym)
				break;

		if(!D(i))
			break; /* finished */

		{
			char *key = D(i)->spel;
			decl **val = dynmap_get(spel_decls, key);

			dynarray_add((void ***)&val, D(i)); /* fine if val is null */

			dynmap_set(spel_decls, key, val);
		}

		D(i)->sym = sym_new(D(i), sym_global);

		fold_decl_global(D(i), globs);

		if(DECL_IS_FUNC(D(i))){
			if(decl_is_definition(D(i))){
				/* gather round, attributes */
				decl **const protos = dynmap_get(spel_decls, D(i)->spel);
				decl **proto_i;
				int is_void = 0;

				for(proto_i = protos; *proto_i; proto_i++){
					decl *proto = *proto_i;

					UCC_ASSERT(type_ref_is(proto->ref, type_ref_func), "not func");

					if(type_ref_is(proto->ref, type_ref_func)->bits.func->args_void)
						is_void = 1;

					if(!decl_is_definition(proto)){
						EOF_WHERE(&D(i)->where,
								decl_attr_append(&D(i)->attr, proto->attr));
					}
				}

				/* if "type ()", and a proto is "type (void)", take the void */
				if(is_void)
					for(proto_i = protos; *proto_i; proto_i++)
						type_ref_is((*proto_i)->ref, type_ref_func)->bits.func->args_void = 1;
			}

			fold_func(D(i));
		}
	}

	/* link declarations with definitions */
	fold_link_decl_defs(spel_decls);

	dynmap_free(spel_decls);

	/* static assertions */
	{
		static_assert **i;
		for(i = globs->static_asserts; i && *i; i++){
			static_assert *sa = *i;
			intval val;
			enum constyness const_type;

			FOLD_EXPR(sa->e, sa->scope);
			if(!type_ref_is_integral(sa->e->tree_type))
				DIE_AT(&sa->e->where, "static assert: not an integral expression (%s)", sa->e->f_str());

			const_fold(sa->e, &val, &const_type);

			if(const_type == CONST_NO)
				DIE_AT(&sa->e->where, "static assert: not a constant expression (%s)", sa->e->f_str());

			if(!val.val)
				DIE_AT(&sa->e->where, "static assertion failure: %s", sa->s);
		}
	}

#undef D
}
