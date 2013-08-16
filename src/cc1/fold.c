#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "defs.h"
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
#include "out/lbl.h"

decl     *curdecl_func;
type_ref *curdecl_ref_func_called; /* for funcargs-local labels and return type-checking */

/* FIXME: don't have the callers do type_ref_to_str() */
int fold_type_ref_equal(
		type_ref *a, type_ref *b, where *w,
		enum warning warn, enum decl_cmp extra_flags,
		const char *errfmt, ...)
{
	enum decl_cmp flags = extra_flags | DECL_CMP_ALLOW_VOID_PTR;

	if(!type_ref_is(a, type_ref_ptr) && !type_ref_is(b, type_ref_ptr))
		flags |= DECL_CMP_ALLOW_SIGNED_UNSIGNED;

	/* stronger checks for blocks, functions and and (non-void) pointers */
	if(type_ref_is_nonvoid_ptr(a)
	&& type_ref_is_nonvoid_ptr(b))
	{
		flags |= DECL_CMP_EXACT_MATCH;
	}
	else
	if(type_ref_is(a, type_ref_block)
	|| type_ref_is(b, type_ref_block)
	|| type_ref_is(a, type_ref_func)
	|| type_ref_is(b, type_ref_func))
	{
		flags |= DECL_CMP_EXACT_MATCH;
	}

	if(type_ref_equal(a, b, flags)){
		return 1;
	}else{
		int one_struct;
		va_list l;

		if(fopt_mode & FOPT_PLAN9_EXTENSIONS){
			/* allow b to be an anonymous member of a */
			struct_union_enum_st *a_sue = type_ref_is_s_or_u(type_ref_is_ptr(a)),
													 *b_sue = type_ref_is_s_or_u(type_ref_is_ptr(b));

			if(a_sue && b_sue /* they aren't equal */){
				/* b_sue has an a_sue,
				 * the implicit cast adjusts to return said a_sue */
				if(struct_union_member_find_sue(b_sue, a_sue))
					goto fin;
			}
		}

		/*cc1_warn_at(w, 0, 0, warn, "%s vs. %s for...", decl_to_str(a), decl_to_str_r(buf, b));*/

		one_struct = type_ref_is_s_or_u(a) || type_ref_is_s_or_u(b);

		va_start(l, errfmt);
		cc1_warn_atv(w, one_struct || type_ref_is_void(a) || type_ref_is_void(b), 1, warn, errfmt, l);
		va_end(l);
	}
fin:
	return 0;
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


void fold_check_restrict(expr *lhs, expr *rhs, const char *desc, where *w)
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

			if(!type_ref_equal(decayed, r, DECL_CMP_EXACT_MATCH))
				imp_cast = expr_new_cast(decayed, 1);
		);

	if(imp_cast){
		imp_cast->expr = e;
		fold_expr_cast_descend(imp_cast, stab, 0);
		e = imp_cast;
	}

	return e;
}

void fold_type_ref(type_ref *r, type_ref *parent, symtable *stab)
{
	enum type_qualifier q_to_check = qual_none;

	if(!r || r->folded)
		return;

	r->folded = 1;

	switch(r->type){
		case type_ref_array:
			if(type_ref_is(r->ref, type_ref_func))
				DIE_AT(&r->where, "array of functions");

			if(r->bits.array.size){
				consty k;

				FOLD_EXPR(r->bits.array.size, stab);
				const_fold(r->bits.array.size, &k);

				if(k.type != CONST_VAL)
					DIE_AT(&r->where, "not a numeric constant for array size");
				else if((sintval_t)k.bits.iv.val < 0)
					DIE_AT(&r->where, "negative array size");
				/* allow zero length arrays */
			}
			break;

		case type_ref_func:
			if(type_ref_is(r->ref, type_ref_func))
				DIE_AT(&r->where, "function returning a function");

			if(type_ref_is(parent, type_ref_ptr) && (type_ref_qual(parent) & qual_restrict))
				DIE_AT(&r->where, "restrict qualified function pointer");

			symtab_fold_sues(r->bits.func.arg_scope);
			fold_funcargs(r->bits.func.args, r->bits.func.arg_scope, r);
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
		case type_ref_type:
			break;

		case type_ref_tdef:
		{
			expr *p_expr = r->bits.tdef.type_of;

			/* q_to_check = TODO */
			FOLD_EXPR_NO_DECAY(p_expr, stab);

			if(r->bits.tdef.decl)
				fold_decl(r->bits.tdef.decl, stab, NULL);

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

static int fold_align(int al, int min, int max, where *w)
{
	/* allow zero */
	if(al & (al - 1))
		DIE_AT(w, "alignment %d isn't a power of 2", al);

	UCC_ASSERT(al > 0, "zero align");
	if(al < min)
		DIE_AT(w,
				"can't reduce alignment (%d -> %d)",
				min, al);

	if(al > max)
		max = al;
	return max;
}

static void fold_func_attr(decl *d)
{
	funcargs *fa = type_ref_funcargs(d->ref);

	if(decl_attr_present(d, attr_sentinel) && !fa->variadic)
		WARN_AT(&d->where, "variadic function required for sentinel check");
}

static void fold_decl_add_sym(decl *d, symtable *stab)
{
	/* must be before fold*, since sym lookups are done */
	if(d->sym){
		/* arg */
		UCC_ASSERT(d->sym->type != sym_local || !d->spel /* anon sym, e.g. strk */,
				"sym (type %d) \"%s\" given symbol too early",
				d->sym->type, d->spel);
	}else{
		d->sym = sym_new(d,
				!stab->parent || decl_store_static_or_extern(d->store)
				? sym_global : sym_local);
	}
}

void fold_decl(decl *d, symtable *stab, stmt **pinit_code)
{
#define inits (*pinit_code)
	/* this is called from wherever we can define a
	 * struct/union/enum,
	 * e.g. a code-block (explicit or implicit),
	 *      global scope
	 * and an if/switch/while statement: if((struct A { int i; } *)0)...
	 * an argument list/type_ref::func: f(struct A { int i, j; } *p, ...)
	 */

	decl_attr *attrib = NULL;
	int can_align = 1;

	if(d->folded)
		return;
	d->folded = 1;

	fold_type_ref(d->ref, NULL, stab);

	fold_decl_add_sym(d, stab);

#if 0
	/* if we have a type and it's incomplete, error */
	no - only on use
	if(!type_ref_is_complete(d->ref))
		DIE_AT(&d->where, "use of incomplete type - %s (%s)", d->spel, decl_to_str(d));
#endif

	if(d->field_width){
		consty k;

		FOLD_EXPR(d->field_width, stab);
		const_fold(d->field_width, &k);

		if(k.type != CONST_VAL)
			DIE_AT(&d->where, "constant expression required for field width");

		if((sintval_t)k.bits.iv.val < 0)
			DIE_AT(&d->where, "field width must be positive");

		if(k.bits.iv.val == 0){
			/* allow anonymous 0-width bitfields
			 * we align the next bitfield to a boundary
			 */
			if(d->spel)
				DIE_AT(&d->where,
						"none-anonymous bitfield \"%s\" with 0-width",
						d->spel);
		}else{
			const unsigned max = CHAR_BIT * type_ref_size(d->ref, &d->where);
			if(k.bits.iv.val > max){
				DIE_AT(&d->where,
						"bitfield too large for \"%s\" (%u bits)",
						decl_to_str(d), max);
			}
		}

		if(!type_ref_is_integral(d->ref))
			DIE_AT(&d->where, "field width on non-integral field %s",
					decl_to_str(d));

		/* FIXME: only warn if "int" specified,
		 * i.e. detect explicit signed/unsigned */
		if(k.bits.iv.val == 1 && type_ref_is_signed(d->ref))
			WARN_AT(&d->where, "1-bit signed field \"%s\" takes values -1 and 0",
					decl_to_str(d));

		can_align = 0;
	}

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

		if(stab->parent){
			if(d->func_code)
				DIE_AT(&d->func_code->where, "nested function %s", d->spel);
			else if((d->store & STORE_MASK_STORE) == store_static)
				DIE_AT(&d->where, "block-scoped function cannot have static storage");
		}

		can_align = 0;

		fold_func_attr(d);

	}else if((d->store & STORE_MASK_EXTRA) == store_inline){
		WARN_AT(&d->where, "inline on non-function");
	}

	if(d->align || (attrib = decl_attr_present(d, attr_aligned))){
		const int tal = type_ref_align(d->ref, &d->where);

		struct decl_align *i;
		int max_al = 0;

		if((d->store & STORE_MASK_STORE) == store_register)
			can_align = 0;

		if(!can_align)
			DIE_AT(&d->where, "can't align %s", decl_to_str(d));

		for(i = d->align; i; i = i->next){
			int al;

			if(i->as_int){
				consty k;

				const_fold(
						FOLD_EXPR(i->bits.align_intk, stab),
						&k);

				if(k.type != CONST_VAL)
					DIE_AT(&d->where, "alignment must be an integer constant");

				al = k.bits.iv.val;
			}else{
				type_ref *ty = i->bits.align_ty;
				UCC_ASSERT(ty, "no type");
				fold_type_ref(ty, NULL, stab);
				al = type_ref_align(ty, &d->where);
			}

			if(al == 0)
				al = decl_size(d);
			max_al = fold_align(al, tal, max_al, &d->where);
		}

		if(attrib){
			unsigned long al;

			if(attrib->attr_extra.align){
				consty k;

				FOLD_EXPR(attrib->attr_extra.align, stab);
				const_fold(attrib->attr_extra.align, &k);

				if(k.type != CONST_VAL)
					DIE_AT(&attrib->where, "aligned attribute not reducible to integer constant");
				al = k.bits.iv.val;
			}else{
				al = platform_align_max();
			}

			max_al = fold_align(al, tal, max_al, &attrib->where);
			if(!d->align)
				d->align = umalloc(sizeof *d->align);
		}

		d->align->resolved = max_al;
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

		/* don't generate for anonymous symbols
		 * they're done elsewhere (e.g. compound literals)
		 */
		if(d->spel && pinit_code){
			/* this creates the below s->inits array */
			if((d->store & STORE_MASK_STORE) == store_static){
				fold_decl_global_init(d, stab);
			}else{
				EOF_WHERE(&d->where,
						if(!inits)
							inits = stmt_new_wrapper(code, symtab_new(stab));

						decl_init_brace_up_fold(d, inits->symtab);
						decl_init_create_assignments_base(d->init,
							d->ref, expr_new_identifier(d->spel),
							inits);
					);
				/* folded elsewhere */
			}
		}
	}

	/* name static decls */
	if(stab->parent
	&& (d->store & STORE_MASK_STORE) == store_static
	&& d->spel)
	{
			d->spel_asm = out_label_static_local(curdecl_func->spel, d->spel);
	}
#undef inits
}

void fold_decl_global_init(decl *d, symtable *stab)
{
	if(!d->init)
		return;

	EOF_WHERE(&d->where,
		/* this completes the array, if any */
		decl_init_brace_up_fold(d, stab);
	);

	if(!decl_init_is_const(d->init, stab)){
		DIE_AT(&d->init->where, "%s %s initialiser not constant",
				stab->parent ? "static" : "global",
				decl_init_to_str(d->init->type));
	}
}

static void fold_func(decl *func_decl)
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

		if(curdecl_func->store & store_inline
		&& (curdecl_func->store & STORE_MASK_STORE) == store_default)
		{
			WARN_AT(&curdecl_func->where,
					"pure inline function will not have code emitted "
					"(missing \"static\" or \"extern\")");
		}

		if(curdecl_func->ref->type != type_ref_func)
			WARN_AT(&curdecl_func->where, "typedef function implementation is not C");

		symtab_add_args(
				func_decl->func_code->symtab,
				fref->bits.func.args,
				func_decl->spel);

		fold_stmt(func_decl->func_code);

		if(decl_attr_present(curdecl_func, attr_noreturn)){
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

void fold_decl_global(decl *d, symtable *stab)
{
	switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
		case store_extern:
		case store_default:
		case store_static:
			break;

		case store_inline: /* allowed, but not accessible via STORE_MASK_STORE */
			ICE("inline");
		case store_typedef: /* global typedef */
			break;

		case store_auto:
		case store_register:
			DIE_AT(&d->where, "invalid storage class %s on global scoped %s",
					decl_store_to_str(d->store),
					DECL_IS_FUNC(d) ? "function" : "variable");
	}

	fold_decl(d, stab, NULL);

	if(DECL_IS_FUNC(d)){
		UCC_ASSERT(!d->init, "function has init?");
		fold_func(d);
	}else{
		/*
		 * inits are normally handled in stmt_code,
		 * but this is global, handle here
		 */
		fold_decl_global_init(d, stab);
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

void fold_disallow_bitfield(expr *e, const char *desc, ...)
{
	if(e && expr_kind(e, struct)){
		decl *d = e->bits.struct_mem.d;

		if(d->field_width){
			va_list l;
			va_start(l, desc);
			vdie(&e->where, 1, desc, l);
			va_end(l);
		}
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
			dynarray_count(st->children),
			dynarray_count(st->decls),
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

	dynarray_add(&t->parent->codes, t);

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
			fold_decl(d, stab, NULL);

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

void fold_merge_tenatives(symtable *stab)
{
	decl **const globs = stab->decls;

	int i;
	/* go in reverse */
	for(i = dynarray_count(globs) - 1; i >= 0; i--){
		decl *d = globs[i];
		decl *init = NULL;

		/* functions are checked via .func_code on parsing */
		if(d->proto_flag || !d->spel || DECL_IS_FUNC(d))
			continue;

		switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
			case store_typedef:
			case store_extern:
				continue;
			default:
				break;
		}

		/* check for a single init between all prototypes */
		for(; d; d = d->proto){
			d->proto_flag = 1;
			if(d->init){
				if(init){
					char wbuf[WHERE_BUF_SIZ];
					DIE_AT(&init->where, "multiple definitions of \"%s\"\n"
							"%s: note: other definition here", init->spel,
							where_str_r(wbuf, &d->where));
				}
				init = d;
			}else if(DECL_IS_ARRAY(d) && type_ref_is_complete(d->ref)){
				init = d;
				decl_default_init(d, stab);
			}
		}
		if(!init){
			/* no initialiser, give the last declared one a default init */
			d = globs[i];

			decl_default_init(d, stab);

			if(DECL_IS_ARRAY(d)){
				WARN_AT(&d->where,
						"tenative array definition assumed to have one element");
			}

			cc1_warn_at(&d->where, 0, 1, WARN_TENATIVE_INIT,
					"default-initialising tenative definition of \"%s\"",
					d->spel);
		}
	}
}
