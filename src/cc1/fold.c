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
#include "ops/__builtin.h"

decl *curdecl_func, *curdecl_func_called; /* for funcargs-local labels and return type-checking */

static where asm_struct_enum_where;

void fold_decl_equal(decl *a, decl *b, where *w, enum warning warn,
		const char *errfmt, ...)
{
	if(!decl_equal(a, b, DECL_CMP_ALLOW_VOID_PTR | (fopt_mode & FOPT_STRICT_TYPES ? DECL_CMP_STRICT_PRIMITIVE : 0))){
		int one_struct;
		char buf[DECL_STATIC_BUFSIZ];
		va_list l;

		cc1_warn_at(w, 0, 0, warn, "%s vs. %s for...", decl_to_str(a), decl_to_str_r(buf, b));


		one_struct = (!a->desc && a->type->sue && a->type->sue->primitive != type_enum)
			        || (!b->desc && b->type->sue && b->type->sue->primitive != type_enum);

		va_start(l, errfmt);
		cc1_warn_atv(w, one_struct || decl_is_void(a) || decl_is_void(b), 1, warn, errfmt, l);
		va_end(l);
	}
}

void fold_insert_casts(decl *dlhs, expr **prhs, symtable *stab, where *w, const char *desc)
{
	expr *rhs = *prhs;

	if(!decl_equal(dlhs, rhs->tree_type, 1)){
		/* insert a cast: rhs -> lhs */
		if(expr_kind(rhs, val) && decl_is_integral(rhs->tree_type)){
			/* don't cast - just change the tree_type */
			rhs->tree_type->type->primitive = dlhs->type->primitive;
			rhs->tree_type->type->is_signed = dlhs->type->is_signed;
			rhs->tree_type->type->qual      = dlhs->type->qual;
		}else{
			expr *cast;

			cast = expr_new_cast(decl_copy_keep_array(dlhs), 1);
			cast->expr = rhs;
			*prhs = cast;

			/* need to fold the cast again - mainly for "loss of precision" warning */
			fold_expr_cast_descend(cast, stab, 0);
		}
	}

#define lhs_signed dlhs->type->is_signed
#define rhs_signed rhs->tree_type->type->is_signed

	if(rhs_signed != lhs_signed){
		if(expr_kind(rhs, val) && rhs->val.iv.val >= 0){
			rhs->tree_type->type->is_signed = 0;
		}else{
#define SPEL_IF_IDENT(hs)                          \
				expr_kind(hs, identifier) ? " ("     : "", \
				expr_kind(hs, identifier) ? hs->spel : "", \
				expr_kind(hs, identifier) ? ")"      : ""

			cc1_warn_at(w, 0, 1, WARN_SIGN_COMPARE,
					"operation between signed and unsigned%s%s%s in %s",
					SPEL_IF_IDENT(rhs), desc);
		}
	}
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

void fold_expr(expr *e, symtable *stab)
{
	where *old_w;

	if(e->tree_type)
		return;

	fold_get_sym(e, stab);

	old_w = eof_where;
	eof_where = &e->where;
	e->f_fold(e, stab);
	eof_where = old_w;

	UCC_ASSERT(e->tree_type, "no tree_type after fold (%s)", e->f_str());
	UCC_ASSERT(e->tree_type->type->primitive != type_unknown, "unknown type after folding expr %s", e->f_str());
}

void fold_decl_desc(decl_desc *dp, symtable *stab, decl *root)
{
	switch(dp->type){
		case decl_desc_func:
			fold_funcargs(dp->bits.func, stab, root->spel);
			break;

		case decl_desc_array:
		{
			intval sz;

			fold_expr(dp->bits.array_size, stab);
			const_fold_need_val(dp->bits.array_size, &sz);

			if(sz.val < 0)
				DIE_AT(&dp->where, "negative array length %ld", sz.val);

			if(sz.val == 0 && !root->init && root->type->store != store_extern)
				DIE_AT(&dp->where, "incomplete array");
		}

		case decl_desc_block:
			/* TODO? */
		case decl_desc_ptr:
			/* TODO: check qual */
			break;
	}

	if(dp->child)
		fold_decl_desc(dp->child, stab, root);
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
			where *old_w = eof_where;
			eof_where = &asm_struct_enum_where;
			m->val = expr_new_val(defval);
			eof_where = old_w;

			if(bitmask)
				defval <<= 1;
			else
				defval++;

		}else{
			intval iv;

			fold_expr(e, stab);
			const_fold_need_val(e, &iv);

			defval = bitmask ? iv.val << 1 : iv.val + 1;
		}
	}
}

int fold_sue(struct_union_enum_st *sue, symtable *stab)
{
	int offset;
	sue_member **i;

	if(sue->primitive == type_enum){
		fold_enum(sue, stab);
		offset = platform_word_size(); /* XXX: assumes enums are 64-bit */
	}else{
		for(offset = 0, i = sue->members; i && *i; i++){
			decl *d = (*i)->struct_member;

			fold_decl(d, stab);

			if(sue->primitive == type_struct)
				d->struct_offset = offset;
			/* else - union, all offsets are the same */

			if(d->type->sue && !decl_is_ptr(d)){
				if(d->type->sue == sue)
					DIE_AT(&d->where, "nested %s", sue_str(sue));

				offset += fold_sue(d->type->sue, stab);
			}else{
				offset += decl_size(d);
			}
		}
	}

	return offset;
}

void fold_gen_init_assignment2(expr *base, decl *dfor, decl_init *init_from, stmt *codes)
{
	/* assignment expr for each init */
	const int ninits = decl_init_len(init_from);

	if(decl_has_incomplete_array(dfor)){
		/* case 1: int x[][2] = { 0, 1, 2, 3 } - complete to 2
		 * case 2: int x[][2] = { {1}, {2} } - complete to 2
		 */
		int complete_to;

		UCC_ASSERT(init_from->type == decl_init_brace, "not a brace initialiser");

		/* decide based on the first sub */
		switch(init_from->bits.inits[0]->type){
			default:
				ICE("invalid initialiser");

			case decl_init_scalar:
			{
				decl *dtmp = decl_ptr_depth_dec(decl_copy_keep_array(dfor), &dfor->where);
				/* (int[2] size = 8) / type_size = 2 */
				complete_to = ninits / (decl_size(dtmp) / type_size(dtmp->type));

				fprintf(stderr, "completing array (subtype %s) to %d\n",
						decl_to_str(dtmp), complete_to);

				decl_free(dtmp);

				WARN_AT(&init_from->where, "missing braces around initialiser");
				break;
			}

			case decl_init_brace:
				complete_to = ninits;
				break;
		}

		decl_complete_array(dfor, complete_to);
	}

	if(decl_is_array(dfor)){
		const int pull_from_this_init = init_from->bits.inits[0]->type == decl_init_scalar;
		const int this_array_count = decl_inner_array_count(dfor);
		const int stride = ninits / this_array_count;
		int i;
		decl *darray_deref;

		darray_deref = decl_ptr_depth_dec(decl_copy_keep_array(dfor), &dfor->where);

		fprintf(stderr, "stride %d (ninits = %d / this_array = %d) for %s\n",
				stride, ninits, this_array_count, decl_to_str(darray_deref));

		for(i = 0; i < this_array_count; i++){
			expr *target;
			decl_init *elem_init;

			target = expr_new_op(op_plus);
			target->lhs = base;
			target->rhs = expr_new_val(i); /* access the i'th element of base */

			if(pull_from_this_init){
				int sub_idx;

				elem_init = decl_init_new(decl_init_brace);

				fprintf(stderr, "for sub_idx = %d, < %d\n", i * stride, stride);

				for(sub_idx = i * stride; sub_idx < stride; sub_idx++){
					decl_init *new = decl_init_new(decl_init_scalar);
					decl_init *indexed_expr;

					memcpy(&new->where, &elem_init->where, sizeof new->where);

					dynarray_add((void ***)&elem_init->bits.inits, new);

					indexed_expr = init_from->bits.inits[sub_idx];

					/* this will abort for e.g. int x[] = { 0, { 1, 2 } }; */
					UCC_ASSERT(indexed_expr->type == decl_init_scalar, "not scalar subinit");

					new->bits.expr = indexed_expr->bits.expr;
				}
			}else{
				elem_init = init_from->bits.inits[i];
			}

			fprintf(stderr, "next level init from %s\n", decl_init_to_str(init_from->type));
			fold_gen_init_assignment2(target, darray_deref, elem_init, codes);

			if(pull_from_this_init){
				ICW("need to free elem_init");
			}
		}

		decl_free(darray_deref);
	}else{
		/* scalar init */
		expr *assign_init;

		switch(init_from->type){
			case decl_init_scalar:
				assign_init = expr_new_assign(expr_new_deref(base), init_from->bits.expr);

				dynarray_prepend((void ***)&codes->codes,
						expr_to_stmt(assign_init, codes->symtab));
				break;

			case decl_init_brace:
				if(ninits > 1)
					WARN_AT(&init_from->where, "excess initialisers for scalar");

				fold_gen_init_assignment2(base, dfor, init_from->bits.inits[0], codes);
				break;
		}
	}
}

void fold_gen_init_assignment(decl *dfor, stmt *code)
{
	expr *base = expr_new_identifier(dfor->spel);

	fold_gen_init_assignment2(base, dfor, dfor->init, code);
}

#if 0
void fold_decl_init(decl *for_decl, decl_init *di, symtable *stab)
{
	fold_gen_init_assignment(for_decl, codes);

	/* fold + type check for statics + globals */
	if(decl_is_array(for_decl)){
		/* don't allow scalar inits */
		switch(di->type){
			case decl_init_struct:
			case decl_init_scalar:
				DIE_AT(&for_decl->where, "can't initialise array decl with scalar or struct");

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
				/*ICE("TODO: struct init with scalar");*/
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

				fprintf(stderr, "checked decl struct init for %s\n", for_decl->spel);
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
					DIE_AT(&for_decl->where, "initialisation of incomplete struct");
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

					ICE(".x struct init");

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

void fold_decl(decl *d, symtable *stab)
{
	decl_desc *dp;

	/* typedef / __typeof folding */
	while(d->type->typeof){
		/* get the typedef decl from t->decl->tree_type */
		const enum type_qualifier old_qual  = d->type->qual;
		const enum type_storage   old_store = d->type->store;
		decl *from;
		expr *type_exp;

		type_exp = d->type->typeof;

		fold_expr(type_exp, stab);

		/* either get the typeof() from the decl or the expr type */
		from = d->type->typeof->decl;
		if(!from)
			from = d->type->typeof->expr->tree_type;

		UCC_ASSERT(from, "no decl for typeof/typedef fold: "
				".decl = %p, .expr->tt = %p",
				(void *)d->type->typeof->decl,
				(void *)d->type->typeof->expr->tree_type);

		decl_free(type_exp->tree_type);
		type_exp->tree_type = decl_copy(from);

		/* type */
		memcpy(d->type, from->type, sizeof *d->type);
		d->type->qual  |= old_qual;
		d->type->store  = old_store;

		/* decl */
		if(from->desc){
			decl_desc *ins = decl_desc_copy(from->desc, 0);

			decl_desc_append(&ins, d->desc);
			d->desc = ins;
		}

		/* attr */
		decl_attr_append(&d->attr, from->attr);
	}
	decl_desc_link(d);

	UCC_ASSERT(d->type && d->type->store != store_typedef, "typedef store after tdef folding");

	/* check for array of funcs, func returning array */
	for(dp = decl_desc_tail(d); dp; dp = dp->parent_desc){

		if(dp->parent_desc && dp->parent_desc->type == decl_desc_func){
			if(dp->type == decl_desc_array)
				DIE_AT(&dp->where, "can't have an array of functions");
			else if(dp->type == decl_desc_func)
				DIE_AT(&dp->where, "can't have a function returning a function");
		}

		if(dp->type == decl_desc_block
		&& (!dp->parent_desc || dp->parent_desc->type != decl_desc_func))
		{
			DIE_AT(&dp->where, "invalid block pointer - function required (got %s)",
					decl_desc_to_str(dp->parent_desc->type));
		}
	}

	/* append type's attr into the decl */
	decl_attr_append(&d->attr, d->type->attr);

	switch(d->type->primitive){
		case type_void:
			if(!decl_is_ptr(d) && !decl_is_callable(d) && d->spel)
				DIE_AT(&d->where, "can't have a void variable - %s (%s)", d->spel, decl_to_str(d));
			break;

		case type_struct:
		case type_union:
			/* don't apply qualifiers to the sue */
		case type_enum:
			if(sue_incomplete(d->type->sue) && !decl_is_ptr(d))
				DIE_AT(&d->where, "use of %s%s%s",
						type_to_str(d->type),
						d->spel ?     " " : "",
						d->spel ? d->spel : "");
			break;

		case type_int:
		case type_char:
		case type__Bool:
		case type_short:
		case type_long:
		case type_float:
		case type_double:
		case type_ldouble:
		case type_llong:
			break;

		case type_unknown:
			ICE("unknown type");
	}

	/*
	 * now we've folded, check for restrict
	 * since typedef int *intptr; intptr restrict a; is valid
	 */
	if(d->desc){
		fold_decl_desc(d->desc, stab, d);
	}else if(d->type->qual & qual_restrict){
		DIE_AT(&d->where, "restrict on non-pointer type %s%s%s",
				type_to_str(d->type),
				d->spel ? " " : "",
				d->spel ? d->spel : "");
	}

	if(d->field_width){
		if(!decl_is_integral(d))
			DIE_AT(&d->where, "field width on non-integral type %s", decl_to_str(d));

		if(d->field_width == 1 && d->type->is_signed)
			WARN_AT(&d->where, "%s 1-bit field width is signed (-1 and 0)", decl_to_str(d));
	}


	if(decl_is_func(d)){
		switch(d->type->store){
			case store_register:
			case store_auto:
				DIE_AT(&d->where, "%s storage for function", type_store_to_str(d->type->store));
			default:
				break;
		}

		if(!d->func_code){
			/* prototype - set extern, so we get a symbol generated (if needed) */
			switch(d->type->store){
				case store_default:
					d->type->store = store_extern;
				case store_extern:
				default:
					break;
			}
		}
	}else{
		if(d->type->is_inline)
			WARN_AT(&d->where, "inline on non-function%s%s",
					d->spel ? " " : "",
					d->spel ? d->spel : "");
	}

	if(d->init){
		if(d->type->store == store_extern){
			/* allow for globals - remove extern since it's a definition */
			if(stab->parent){
				DIE_AT(&d->where, "externs can't be initialised");
			}else{
				WARN_AT(&d->where, "extern initialisation");
				d->type->store = store_default;
			}
		}
	}
}

void fold_decl_global(decl *d, symtable *stab)
{
	switch(d->type->store){
		case store_extern:
		case store_default:
		case store_static:
			break;

		case store_typedef:
			ICE("typedef store");

		case store_auto:
		case store_register:
			DIE_AT(&d->where, "invalid storage class %s on global scoped %s",
					type_store_to_str(d->type->store),
					decl_is_func(d) ? "function" : "variable");
	}

	fold_decl(d, stab);
}

void fold_symtab_scope(symtable *stab)
{
	struct_union_enum_st **sit;

	for(sit = stab->sues; sit && *sit; sit++)
		fold_sue(*sit, stab);
}

void fold_need_expr(expr *e, const char *stmt_desc, int is_test)
{
	if(!decl_is_ptr(e->tree_type) && e->tree_type->type->primitive == type_void)
		DIE_AT(&e->where, "%s requires non-void expression", stmt_desc);

	if(!e->in_parens && expr_kind(e, assign))
		cc1_warn_at(&e->where, 0, 1, WARN_TEST_ASSIGN, "testing an assignment in %s", stmt_desc);

	if(is_test && !decl_is_bool(e->tree_type))
		cc1_warn_at(&e->where, 0, 1, WARN_TEST_BOOL, "testing a non-boolean expression, %s, in %s",
				decl_to_str(e->tree_type), stmt_desc);

	fold_disallow_st_un(e, stmt_desc);
}

void fold_disallow_st_un(expr *e, const char *desc)
{
	if(decl_is_struct_or_union(e->tree_type)){
		DIE_AT(&e->where, "%s involved in %s",
				sue_str(e->tree_type->type->sue),
				desc);
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
			decl_conv_array_func_to_ptr(d); /* must be before the decl is folded (since fold checks this) */

			fold_decl(d, stab);

			if(type_store_static_or_extern(d->type->store)){
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
		curdecl_func_called = decl_func_deref(decl_copy(curdecl_func), NULL);

		symtab_add_args(
				func_decl->func_code->symtab,
				decl_desc_tail(func_decl)->bits.func,
				func_decl->spel);

		fold_stmt(func_decl->func_code);

		if(decl_attr_present(curdecl_func->attr, attr_noreturn)){
			if(!decl_is_void(curdecl_func_called)){
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

		}else if(!decl_is_void(curdecl_func_called)){
			/* non-void func - check it doesn't return */
			if(fold_passable(func_decl->func_code)){
				cc1_warn_at(&func_decl->where, 0, 1, WARN_RETURN_UNDEF,
						"control reaches end of non-void function %s",
						func_decl->spel);
			}
		}

		decl_free(curdecl_func_called);
		curdecl_func_called = NULL;
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

		count_inline = d->type->is_inline;
		count_extern = count_static = 0;
		first_none_extern = NULL;

		switch(d->type->store){
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
			if(!decl_equal(d, e, DECL_CMP_STRICT_PRIMITIVE))
				DIE_AT(&e->where, "mismatching declaration of %s (%s)", d->spel, where_str_r(wbuf, &d->where));

			if(decl_is_definition(e)){
				/* e is the implementation/instantiation */

				if(definition){
					/* already got one */
					DIE_AT(&e->where, "duplicate definition of %s (%s)", d->spel, where_str_r(wbuf, &d->where));
				}

				definition = e;
			}

			count_inline += e->type->is_inline;

			switch(e->type->store){
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

		if(decl_is_func(definition)){
			/*
			 * inline semantics
			 *
			 * all "inline", none "extern" = inline_only
			 * "static inline" = code emitted, decl is static
			 * one "inline", and "extern" mentioned, or "inline" not mentioned = code emitted, decl is extern
			 */
			definition->type->is_inline = count_inline > 0;


			/* all defs must be static, except the def, which is allowed to be non-static */
			if(count_static > 0){
				definition->type->store = store_static;

				if(count_static != count_total && (definition->func_code ? count_static != count_total - 1 : 0)){
					DIE_AT(&definition->where,
							"static/non-static mismatch of function %s (%d static defs vs %d total)",
							definition->spel, count_static, count_total);
				}
			}


			if(definition->type->store == store_static){
				/* static inline */

			}else if(count_inline == count_total && count_extern == 0){
				/* inline only */
				definition->inline_only = 1;
				WARN_AT(&definition->where, "definition is inline-only (ucc doesn't inline currently)");
			}else if(count_inline > 0 && (count_extern > 0 || count_inline < count_total)){
				/* extern inline */
				definition->type->store = store_extern;
			}

			if(definition->type->is_inline && !definition->func_code)
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

static void add_builtins(symtable *globs)
{
	decl **i, **start;

	for(start = i = builtin_funcs(); i && *i; i++){
		decl *d = *i;

		symtab_add(globs, d, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);
	}

	dynarray_free((void ***)&start, NULL);
}

void fold(symtable *globs)
{
#define D(x) globs->decls[x]
	extern const char *current_fname;
	dynmap *spel_decls;
	int i;

	memset(&asm_struct_enum_where, 0, sizeof asm_struct_enum_where);
	asm_struct_enum_where.fname = current_fname;

	add_builtins(globs);

	if(fopt_mode & FOPT_ENABLE_ASM){
		decl *df;
		funcargs *fargs;
		where *old_w;

		old_w = eof_where;
		eof_where = &asm_struct_enum_where;

		df = decl_new();
		decl_set_spel(df, ustrdup(ASM_INLINE_FNAME));

		df->type->primitive = type_int;

		fargs = funcargs_new();
		fargs->arglist    = umalloc(2 * sizeof *fargs->arglist);
		fargs->arglist[0] = decl_new();
		fargs->arglist[1] = NULL;
		fargs->arglist[0]->type->primitive = type_char;
		fargs->arglist[0]->type->qual      = qual_const;
		fargs->arglist[0]->desc            = decl_desc_ptr_new(fargs->arglist[0], NULL);

		df->desc = decl_desc_func_new(df, NULL);
		df->desc->bits.func = fargs;

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

		if(decl_is_func(D(i))){
			if(decl_is_definition(D(i))){
				/* gather round, attributes */
				decl **protos;

				for(protos = dynmap_get(spel_decls, D(i)->spel); *protos; protos++){
					decl *d = *protos;

					if(!decl_is_definition(d)){
						decl_attr_append(&D(i)->attr, d->attr);
					}
				}
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

			fold_expr(sa->e, sa->scope);
			if(!decl_is_integral(sa->e->tree_type))
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
