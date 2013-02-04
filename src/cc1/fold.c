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
	enum decl_cmp flags = DECL_CMP_ALLOW_VOID_PTR | DECL_CMP_ALLOW_SIGNED_UNSIGNED;

	/* stronger checks for blocks and pointers */
	if(type_ref_is(a, type_ref_block) || type_ref_is(b, type_ref_block)
	|| type_ref_is(a, type_ref_ptr)   || type_ref_is(b, type_ref_ptr))
	{
		flags |= DECL_CMP_EXACT_MATCH;
	}

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
		cc1_warn_at(w, 0, 1, WARN_SIGN_COMPARE,
				"operation between signed and unsigned in %s", desc);
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

sym *fold_inc_writes_if_sym(expr *e, symtable *stab)
{
	if(expr_kind(e, identifier)){
		sym *sym = symtab_search(stab, e->bits.ident.spel);

		if(sym){
			sym->nwrites++;
			return sym;
		}
	}

	return NULL;
}

void FOLD_EXPR_NO_DECAY(expr *e, symtable *stab)
{
	if(e->tree_type)
		return;

	EOF_WHERE(&e->where, e->f_fold(e, stab));

	UCC_ASSERT(e->tree_type, "no tree_type after fold (%s)", e->f_str());
}

expr *fold_expr(expr *e, symtable *stab)
{
	/* perform array decay and pointer decay */
	type_ref *r;
	expr *imp_cast = NULL;

	FOLD_EXPR_NO_DECAY(e, stab);

	r = e->tree_type;

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

	return e;
}

void fold_enum(struct_union_enum_st *en, symtable *stab)
{
	const int has_bitmask = !!decl_attr_present(en->attr, attr_enum_bitmask);
	sue_member **i;
	int defval = has_bitmask;

	for(i = en->members; *i; i++){
		enum_member *m = (*i)->enum_member;
		expr *e = m->val;

		/* -1 because we can't do dynarray_add(..., 0) */
		if(e == (expr *)-1){

			EOF_WHERE(&asm_struct_enum_where,
				m->val = expr_new_val(defval)
			);

			if(has_bitmask)
				defval <<= 1;
			else
				defval++;

		}else{
			intval iv;

			FOLD_EXPR(e, stab);
			const_fold_need_val(e, &iv);
			m->val = e;

			defval = has_bitmask ? iv.val << 1 : iv.val + 1;
		}
	}
}

int fold_sue(struct_union_enum_st *const sue, symtable *stab)
{
	if(sue->size)
		return sue->size;

	if(sue->primitive == type_enum){
		fold_enum(sue, stab);

		return sue->size = sue_size(sue, &sue->where);

	}else{
		int align_max = 1;
		int offset = 0;
		sue_member **i;

		if(decl_attr_present(sue->attr, attr_packed))
			ICE("TODO: __attribute__((packed)) support");

		for(i = sue->members; i && *i; i++){
			decl *d = (*i)->struct_member;
			int align, sz;
			struct_union_enum_st *sub_sue;

			fold_decl(d, stab);

			if((sub_sue = type_ref_is_s_or_u_or_e(d->ref))){
				if(sub_sue != sue)
					fold_sue(sub_sue, stab);

				if(type_ref_is(d->ref, type_ref_ptr) || sub_sue->primitive == type_enum)
					goto normal;

				if(sub_sue == sue)
					DIE_AT(&d->where, "nested %s", sue_str(sue));

				sz = sue_size(sub_sue, &d->where);
				align = sub_sue->align;

			}else{
normal:
				/* for now - align = sz */
				align = sz = decl_size(d, &d->where);
			}


			{
				int after_space;

				pack_next(&offset, &after_space, sz, align);
				/* offset is the end of the decl, after_space is the start */

				if(sue->primitive == type_struct)
					d->struct_offset = after_space;
				/* else - union, all offsets are the same */

				if(align > align_max)
					align_max = align;
			}
		}

		sue->align = align_max;
		return sue->size = offset;
	}
}

void fold_type_ref(type_ref *r, type_ref *parent, symtable *stab)
{
	enum type_qualifier q_to_check = qual_none;

	if(!r || r->folded)
		return;

	r->folded = 1;

	switch(r->type){
	/* check for array of funcs, func returning array */
		case type_ref_array:
		{
			consty k;

			if(type_ref_is(r->ref, type_ref_func))
				DIE_AT(&r->where, "array of functions");

			FOLD_EXPR(r->bits.array_size, stab);
			const_fold(r->bits.array_size, &k);

			if(k.type != CONST_VAL)
				DIE_AT(&r->where, "not a numeric constant for array size");
			else if(k.bits.iv.val < 0)
				DIE_AT(&r->where, "negative array size");
			/* allow zero length arrays */
			break;
		}

		case type_ref_func:
			if(type_ref_is(r->ref, type_ref_func))
				DIE_AT(&r->where, "function returning a function");

			if(type_ref_is(parent, type_ref_ptr) && (type_ref_qual(parent) & qual_restrict))
				DIE_AT(&r->where, "restrict qualified function pointer");

			fold_funcargs(r->bits.func, stab, r);
			break;

		case type_ref_block:
			if(!type_ref_is(r->ref, type_ref_func))
				DIE_AT(&r->where, "invalid block pointer - function required (got %s)",
						type_ref_to_str(r->ref));

			/*q_to_check = r->bits.block.qual; - allowed */
			break;

		case type_ref_cast:
			if(!r->bits.cast.is_signed_cast)
				q_to_check = type_ref_qual(r);
			break;

		case type_ref_ptr:
			/*q_to_check = r->bits.qual; - allowed */
			break;

		case type_ref_type:
			break;

		case type_ref_tdef:
		{
			expr *p_expr = r->bits.tdef.type_of;

			/* q_to_check = TODO */
			FOLD_EXPR_NO_DECAY(p_expr, stab);

			if(r->bits.tdef.decl)
				fold_decl(r->bits.tdef.decl, stab);

			break;
		}
	}

	/*
	 * now we've folded, check for restrict
	 * since typedef int *intptr; intptr restrict a; is valid
	 */
	if(q_to_check & qual_restrict)
		WARN_AT(&r->where, "restrict on non-pointer type '%s'", type_ref_to_str(r));

	fold_type_ref(r->ref, r, stab);
}

void fold_decl(decl *d, symtable *stab)
{
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
		switch(d->store & STORE_MASK_STORE){
			case store_register:
			case store_auto:
				DIE_AT(&d->where, "%s storage for function", decl_store_to_str(d->store));
		}

		if(!d->func_code){
			/* prototype - set extern, so we get a symbol generated (if needed) */
			switch(d->store & STORE_MASK_STORE){
				case store_default:
					d->store |= store_extern;
				case store_extern:
					break;
			}
		}
	}else if((d->store & STORE_MASK_EXTRA) == store_inline){
		WARN_AT(&d->where, "inline on non-function");
	}

	if(d->init){
		if((d->store & STORE_MASK_STORE) == store_extern){
			/* allow for globals - remove extern since it's a definition */
			if(stab->parent){
				DIE_AT(&d->where, "externs can't be initialised");
			}else{
				WARN_AT(&d->where, "extern initialisation");
				d->store &= ~store_extern;
			}
		}
	}
}

void fold_decl_global_init(decl *d, symtable *stab)
{
	stmt *assignments;

	if(!d->init)
		return;

	EOF_WHERE(&d->where,
			assignments = stmt_new_wrapper(code, symtab_new(stab));

		/* this completes the array, if any */
		decl_init_create_assignments_for_spel(d, assignments);
	);

	if(!decl_init_is_const(d->init, stab)){
		DIE_AT(&d->init->where, "%s %s initialiser not constant",
				stab->parent ? "static" : "global",
				decl_init_to_str(d->init->type));
	}

	fold_stmt(assignments);
	d->decl_init_code = assignments;
}

void fold_decl_global(decl *d, symtable *stab)
{
	switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
		case store_extern:
		case store_default:
		case store_static:
			break;

		case store_inline: /* allowed, but not accessible via STORE_MASK_STORE */
		case store_typedef:
			ICE("%s store", decl_store_to_str(d->store));

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

	for(sit = stab->sues; sit && *sit; sit++)
		fold_sue(*sit, stab);
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

void fold_funcargs(funcargs *fargs, symtable *stab, type_ref *from)
{
	decl_attr *da;
	unsigned long nonnulls = 0;

	/* check nonnull corresponds to a pointer arg */
	if((da = type_attr_present(from, attr_nonnull)))
		nonnulls = da->attr_extra.nonnull_args;

	if(fargs->arglist){
		/* check for unnamed params and extern/static specs */
		int i;

		for(i = 0; fargs->arglist[i]; i++){
			decl *const d = fargs->arglist[i];

			/* fold before for array checks, etc */
			fold_decl(d, stab);

			/* convert any array definitions and functions to pointers */
			EOF_WHERE(&d->where,
				/* must be before the decl is folded (since fold checks this) */
				if(decl_conv_array_func_to_ptr(d))
					fold_type_ref(d->ref, NULL, stab); /* refold if we converted */
			);

			if(decl_store_static_or_extern(d->store)){
				DIE_AT(&fargs->where, "function argument %d is static or extern", i + 1);
			}

			/* ensure ptr */
			if((nonnulls & (1 << i))
			&& !type_ref_is(d->ref, type_ref_ptr)
			&& !type_ref_is(d->ref, type_ref_block))
			{
				WARN_AT(&fargs->arglist[i]->where, "nonnull attribute applied to non-pointer argument '%s'",
						type_ref_to_str(d->ref));
			}
		}

		if(i == 0 && nonnulls)
			WARN_AT(&fargs->where, "nonnull attribute applied to function with no arguments");
		else if(nonnulls != ~0UL && nonnulls & -(1 << i))
			WARN_AT(&fargs->where, "nonnull attributes above argument index %d ignored", i + 1);
	}else if(nonnulls){
		WARN_AT(&fargs->where, "nonnull attribute on parameterless function");
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

		type_ref *fref;

		curdecl_func = func_decl;
		fref = type_ref_is(curdecl_func->ref, type_ref_func);
		UCC_ASSERT(fref, "not a func");
		curdecl_ref_func_called = type_ref_func_call(fref, NULL);

		if(curdecl_func->ref->type != type_ref_func)
			WARN_AT(&curdecl_func->where, "typedef function implementation is not C");

		symtab_add_args(
				func_decl->func_code->symtab,
				fref->bits.func,
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
		char *asm_rename;

		key = dynmap_key(spel_decls, i);
		if(!key)
			break;

		decls_for_this = dynmap_get(spel_decls, key);
		d = *decls_for_this;

		definition = decl_is_definition(d) ? d : NULL;

		count_inline = d->store & store_inline;
		count_extern = count_static = 0;
		first_none_extern = NULL;
		asm_rename = d->spel_asm;

		switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
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

			/* check asm renames */
			if(e->spel_asm){
				if(asm_rename && strcmp(asm_rename, e->spel_asm))
					WARN_AT(&d->where, "multiple asm renames\n%s", where_str_r(wbuf, &e->where));
				asm_rename = e->spel_asm;
			}

			if(decl_is_definition(e)){
				/* e is the implementation/instantiation */

				if(definition){
					/* already got one */
					DIE_AT(&e->where, "duplicate definition of %s (%s)", d->spel, where_str_r(wbuf, &d->where));
				}

				definition = e;
			}

			count_inline += e->store & store_inline;

			switch((enum decl_storage)(e->store & STORE_MASK_STORE)){
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
			if(count_inline > 0)
				definition->store |= store_inline;


			/* all defs must be static, except the def, which is allowed to be non-static */
			if(count_static > 0){
				definition->store |= store_static;

				if(count_static != count_total && (definition->func_code ? count_static != count_total - 1 : 0)){
					DIE_AT(&definition->where,
							"static/non-static mismatch of function %s (%d static defs vs %d total)",
							definition->spel, count_static, count_total);
				}
			}


			if((definition->store & STORE_MASK_STORE) == store_static){
				/* static inline */

			}else if(count_inline == count_total && count_extern == 0){
				/* inline only */
				definition->inline_only = 1;
				WARN_AT(&definition->where, "definition is inline-only (ucc doesn't inline currently)");
			}else if(count_inline > 0 && (count_extern > 0 || count_inline < count_total)){
				/* extern inline */
				definition->store |= store_extern;
			}

			if((definition->store & store_inline) && !definition->func_code)
				WARN_AT(&definition->where, "inline function missing implementation");

		}else if(count_static && count_static != count_total){
			/* TODO: iter through decls, printing them out */
			DIE_AT(&definition->where, "static/non-static mismatch of %s", definition->spel);
		}

		definition->is_definition = 1;

		if(!definition->spel_asm)
			definition->spel_asm = asm_rename;

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
				type_ref_new_type_qual(type_char, qual_const),
				qual_none);

		df->ref = type_ref_new_func(type_ref_new_INT(), fargs);

		symtab_add(globs, df, sym_global, SYMTAB_NO_SYM, SYMTAB_PREPEND);

		eof_where = old_w;
	}

	fold_symtab_scope(globs);

	if(!globs->decls)
		goto skip_decls;

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

skip_decls:
	/* static assertions */
	{
		static_assert **i;
		for(i = globs->static_asserts; i && *i; i++){
			static_assert *sa = *i;
			consty k;

			FOLD_EXPR(sa->e, sa->scope);
			if(!type_ref_is_integral(sa->e->tree_type))
				DIE_AT(&sa->e->where, "static assert: not an integral expression (%s)", sa->e->f_str());

			const_fold(sa->e, &k);

			if(!is_const(k.type))
				DIE_AT(&sa->e->where, "static assert: not a constant expression (%s)", sa->e->f_str());

			if(!k.bits.iv.val)
				DIE_AT(&sa->e->where, "static assertion failure: %s", sa->s);
		}
	}

#undef D
}
