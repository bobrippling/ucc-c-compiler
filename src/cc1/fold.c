#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "defs.h"
#include "../util/util.h"
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
#include "fold_sue.h"
#include "format_chk.h"
#include "type_is.h"
#include "type_nav.h"

int fold_had_error;

void fold_insert_casts(type *tlhs, expr **prhs, symtable *stab)
{
	/* insert a cast: rhs -> lhs */
	*prhs = expr_set_where(
			expr_new_cast(*prhs, tlhs, 1),
			&(*prhs)->where);

	/* need to fold the cast again - mainly for "loss of precision" warning */
	fold_expr_cast_descend(*prhs, stab, 0);
}

int fold_type_chk_warn(
		type *lhs, type *rhs,
		where *w, const char *desc)
{
	int error = 1;
	const char *detail = "";

	switch(type_cmp(lhs, rhs, TYPE_CMP_ALLOW_TENATIVE_ARRAY)){
		case TYPE_CONVERTIBLE_IMPLICIT:
			/* attempt to insert regardless, e.g. _Bool x = 5;
			 *  - they match but we need the _Bool cast */
			return 1;
		case TYPE_EQUAL:
		case TYPE_QUAL_ADD: /* const int <- int */
		case TYPE_QUAL_SUB: /* int <- const int */
		case TYPE_QUAL_POINTED_ADD: /* const char * <- char * */
		case TYPE_EQUAL_TYPEDEF:
			break;

		case TYPE_QUAL_NESTED_CHANGE: /* char ** <- const char ** or vice versa */
			detail = "nested ";
		case TYPE_QUAL_POINTED_SUB: /* char * <- const char * */
		case TYPE_CONVERTIBLE_EXPLICIT:
			error = 0;

		case TYPE_NOT_EQUAL:
		{
			char buf[TYPE_STATIC_BUFSIZ];
			char wbuf[WHERE_BUF_SIZ];

			(error ? warn_at_print_error : warn_at)(
					w,
					"mismatching %stypes, %s:\n%s: note: '%s' vs '%s'",
					detail, desc, where_str_r(wbuf, w),
					type_to_str_r(buf, lhs),
					type_to_str(       rhs));

			if(error){
				fold_had_error = 1;
				return 0; /* don't cast - we don't want duplicate errors */
			}
			return 1; /* need cast */
		}
	}
	return 0;
}

void fold_type_chk_and_cast(
		type *lhs, expr **prhs,
		symtable *stab, where *w,
		const char *desc)
{
#if 0
	int strict = 0;

	/* stronger checks for blocks, functions and (non-void) pointers */
	if(type_is_nonvoid_ptr(a)
	&& type_is_nonvoid_ptr(b))
	{
		strict = 1;
	}
	else
	if(type_is(a, type_block)
	|| type_is(b, type_block)
	|| type_is(a, type_func)
	|| type_is(b, type_func))
	{
		strict = 1;
	}
#endif
	int cast = 0;

	/* special case - allow assignment to pointer from 0-constant */
	if(type_is_ptr_or_block(lhs) && expr_is_null_ptr(*prhs, NULL_STRICT_INT))
		cast = 1; /* no warning, but still sign extend the zero */
	else if(fold_type_chk_warn(lhs, (*prhs)->tree_type, w, desc))
		cast = 1;

	if(cast)
		fold_insert_casts(lhs, prhs, stab);
}

void fold_check_restrict(expr *lhs, expr *rhs, const char *desc, where *w)
{
	/* restrict operation checks */
	const enum type_qualifier ql = type_qual(lhs->tree_type),
	                          qr = type_qual(rhs->tree_type);

	if((ql & qual_restrict) && (qr & qual_restrict))
		warn_at(w, "restrict pointers in %s", desc);
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

void fold_expr(expr *e, symtable *stab)
{
	if(e->tree_type)
		return;

	e->f_fold(e, stab);

	UCC_ASSERT(e->tree_type, "no tree_type after fold (%s)", e->f_str());
}

static expr *fold_expr_lval2rval(expr *e, symtable *stab)
{
	fold_expr(e, stab);

	if(expr_is_lval(e)){
		e = expr_set_where(
				expr_new_cast_rval(e),
				&e->where);

		fold_expr_cast_descend(e, stab, 0);
	}

	return e;
}

expr *fold_expr_decay(expr *e, symtable *stab)
{
	/* perform array decay and pointer decay */
	type *r;
	type *decayed;

	e = fold_expr_lval2rval(e, stab);

	r = e->tree_type;

	decayed = type_decay(r);

	if(decayed != r){
		expr *imp_cast = expr_set_where(
				expr_new_cast_decay(e, decayed),
				&e->where);
		fold_expr_cast_descend(imp_cast, stab, 0);
		e = imp_cast;
	}

	return e;
}

static void fold_calling_conv(type *r)
{
	enum calling_conv conv;
	attribute *found;

	/* don't currently check for a second one... */
	found = type_attr_present(r, attr_call_conv);

	if(found){
		conv = found->bits.conv;
	}else{
		static int init = 0;
		static enum calling_conv def_conv;

		if(!init){
			init = 1;

			if(platform_32bit()){
				def_conv = conv_cdecl;
			}else{
				/* 64-bit - MS or SYSV */
				if(platform_sys() == PLATFORM_CYGWIN)
					def_conv = conv_x64_ms;
				else
					def_conv = conv_x64_sysv;
			}
		}

		conv = def_conv;
	}

	type_funcargs(r)->conv = conv;
}

void fold_type_w_attr(
		type *const r, type *const parent, where *loc,
		symtable *stab, attribute *attr)
{
	type *thisparent = r;
	attribute *this_attr = NULL;
	enum type_qualifier q_to_check = qual_none;

	/* must be above the .folded check,
	 * since we use the same attribute node for
	 * several attr_ucc_debug instances */
	attribute_debug_check(attr);

	if(!r || r->folded)
		return;
	r->folded = 1;

	switch(r->type){
		case type_auto:
			ICE("__auto_type");

		case type_array:
			if(r->bits.array.size){
				consty k;
				where *array_loc = &r->bits.array.size->where;

				FOLD_EXPR(r->bits.array.size, stab);

				if(r->bits.array.is_vla){
					if(cc1_std < STD_C99){
						warn_at(
								&r->bits.array.size->where,
								"variable length array is a C99 feature");
					}
				}else{
					const_fold(r->bits.array.size, &k);

					UCC_ASSERT(k.type == CONST_NUM,
							"not a constant for array size");

					UCC_ASSERT(K_INTEGRAL(k.bits.num),
							"integral array should be checked during parse");

					if((sintegral_t)k.bits.num.val.i < 0)
						die_at(array_loc, "negative array size");
					/* allow zero length arrays */
					else if(k.nonstandard_const)
						warn_at(&k.nonstandard_const->where,
								"%s-expr is a non-standard constant expression (for array size)",
								k.nonstandard_const->f_str()); /* TODO: VLA here */
				}
			}
			break;

		case type_func:
			/* necessary for struct-scope checks when there's no {} body */
			r->bits.func.arg_scope->are_params = 1;

			symtab_fold_sues(r->bits.func.arg_scope);
			fold_funcargs(r->bits.func.args, r->bits.func.arg_scope, attr);
			fold_calling_conv(r);
			break;

		case type_block:
			/*q_to_check = r->bits.block.qual; - allowed */
			break;

		case type_attr:
			this_attr = r->bits.attr;
			thisparent = parent;
			break;

		case type_where:
			/* nothing to do */
			thisparent = parent;
			break;

		case type_cast:
			if(!r->bits.cast.is_signed_cast)
				q_to_check = type_qual(r);
			thisparent = parent;
			break;

		case type_ptr:
			/*q_to_check = r->bits.qual; - allowed */
			break;

		case type_btype:
		{
			/* check if we're a new struct/union/enum decl
			 * (yes - enums too) */
			struct_union_enum_st *sue = type_is_s_or_u_or_e(r);

			if(sue){
				fold_sue(sue, stab);

				if(stab->are_params){
					struct_union_enum_st *above = sue_find_descend(
							stab->parent, sue->spel, NULL);

					if(!above){
						warn_at(&sue->where,
								"declaration of '%s %s' only visible inside function",
								sue_str(sue), sue->spel);
					}
				}
			}
			break;
		}

		case type_tdef:
		{
			expr *p_expr = r->bits.tdef.type_of;

			/* q_to_check = TODO */
			fold_expr_no_decay(p_expr, stab);

			if(r->bits.tdef.decl)
				fold_decl(r->bits.tdef.decl, stab);

			thisparent = parent;
			break;
		}
	}

	/*
	 * now we've folded, check for restrict
	 * since typedef int *intptr; intptr restrict a; is valid
	 */
	if(q_to_check & qual_restrict){
		if(!type_is_ptr(r)){
			warn_at(loc,
					"restrict on non-pointer type '%s'",
					type_to_str(r));

		}else if(type_is_fptr(r->ref)){
			/* ^ next is a pointer, what's after that? */
			die_at(loc, "restrict qualified function pointer");
		}
	}

	fold_type_w_attr(r->ref, thisparent, loc, stab, this_attr ? this_attr : attr);

	/* checks that rely on r->ref being folded... */
	switch(r->type){
		case type_array:
			if(!type_is_complete(r->ref)){
				fold_had_error = 1;
				warn_at_print_error(loc,
						"array has incomplete type '%s'",
						type_to_str(r->ref));
			}
			/* fall through to x()[] check */

		case type_func:
			if(type_is(r->ref, type_func)){
				fold_had_error = 1;
				warn_at_print_error(loc,
						r->type == type_func
							? "function returning a function"
							: "array of functions");
			}
			break;

		case type_cast:
			if(!r->bits.cast.is_signed_cast
			&& type_is(r->ref, type_func))
			{
				/* C11 6.7.3.9
				 * If the specification of an array type includes any type qualifiers,
				 * the element type is so-qualified, not the array type. If the
				 * specification of a function type includes any type qualifiers, the
				 * behavior is undefined)
				 *
				 * Array types are handled by type_qualify()
				 */
				warn_at(loc, "qualifier on function type '%s'", type_to_str(r->ref));
			}
			break;

		case type_block:
			if(!type_is(r->ref, type_func)){
				die_at(loc,
						"invalid block pointer - function required (got %s)",
						type_to_str(r->ref));
			}
			break;

		default:
			break;
	}


	{
		/* warn on embedded structs/unions with flexarrs */
		struct_union_enum_st *sue = type_is_s_or_u(r);
		if(sue && sue->flexarr
		&& (type_is_array(parent) || type_is_s_or_u(parent)))
		{
			warn_at(&sue->where, "%s with flex-array embedded in %s",
					sue_str(sue), type_to_str(parent));
		}
	}
}

void fold_type(type *t, symtable *stab)
{
	fold_type_w_attr(t, NULL, type_loc(t), stab, NULL);
}

static int fold_align(int al, int min, int max, where *w)
{
	/* allow zero */
	if(al & (al - 1))
		die_at(w, "alignment %d isn't a power of 2", al);

	UCC_ASSERT(al > 0, "zero align");
	if(al < min)
		die_at(w,
				"can't reduce alignment (%d -> %d)",
				min, al);

	if(al > max)
		max = al;
	return max;
}

static void fold_func_attr(decl *d)
{
	funcargs *fa = type_funcargs(d->ref);
	attribute *da;

	if(attribute_present(d, attr_sentinel) && !fa->variadic)
		warn_at(&d->where, "variadic function required for sentinel check");

	if((da = attribute_present(d, attr_format)))
		format_check_decl(d, da);

	if(type_is_void(type_called(d->ref, NULL))
	&& (da = attribute_present(d, attr_warn_unused)))
	{
		warn_at(&d->where, "warn_unused attribute on function returning void");
	}
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
		enum sym_type ty;

		if(stab->are_params){
			ty = sym_arg;
		}else{
			/* no decl_store_duration_is_static() checks here:
			 * we haven't given it a sym yet */
			ty = !stab->parent || decl_store_static_or_extern(d->store)
				? sym_global : sym_local;
		}

		d->sym = sym_new(d, ty);
	}

	if(attribute_present(d, attr_cleanup)
	&& (d->store & STORE_MASK_STORE) != store_typedef
	&& (d->sym->type != sym_local || type_is(d->ref, type_func)))
	{
		warn_at(&d->where, "cleanup attribute only applies to local variables");
	}
}

static void fold_decl_func(decl *d, symtable *stab)
{
	/* allow:
	 *   register int (*f)();
	 * disallow:
	 *   register int   f();
	 *   register int  *f();
	 */
	switch(d->store & STORE_MASK_STORE){
		/* typedef handled elsewhere, since
		 * we may fold before we have .func.code */
		case store_register:
		case store_auto:
			fold_had_error = 1;
			warn_at_print_error(&d->where,
					"%s storage for function",
					decl_store_to_str(d->store));
			break;
	}

	if(stab->parent){
		if(d->bits.func.code)
			die_at(&d->bits.func.code->where, "nested function %s", d->spel);
		else if((d->store & STORE_MASK_STORE) == store_static)
			die_at(&d->where, "block-scoped function cannot have static storage");
	}

	fold_func_attr(d);
}

static void fold_decl_var(decl *d, symtable *stab)
{
	attribute *attrib = NULL;
	int is_static_duration = !stab->parent
		|| (d->store & STORE_MASK_STORE) == store_static;

	if((d->store & STORE_MASK_EXTRA) == store_inline)
		warn_at(&d->where, "inline on non-function");

	if(d->bits.var.align || (attrib = attribute_present(d, attr_aligned))){
		const int tal = type_align(d->ref, &d->where);

		struct decl_align *i;
		int max_al = 0;

		if((d->store & STORE_MASK_STORE) == store_register
		|| d->bits.var.field_width)
		{
			die_at(&d->where, "can't align %s", decl_to_str(d));
		}

		for(i = d->bits.var.align; i; i = i->next){
			int al;

			if(i->as_int){
				consty k;

				const_fold(
						FOLD_EXPR(i->bits.align_intk, stab),
						&k);

				if(k.type != CONST_NUM)
					die_at(&d->where, "alignment must be a constant");
				if(K_FLOATING(k.bits.num))
					die_at(&d->where, "non-integral alignment");

				al = k.bits.num.val.i;
			}else{
				type *ty = i->bits.align_ty;
				UCC_ASSERT(ty, "no type");
				fold_type_w_attr(ty, NULL, type_loc(d->ref), stab, d->attr);
				al = type_align(ty, &d->where);
			}

			if(al == 0)
				al = decl_size(d);
			max_al = fold_align(al, tal, max_al, &d->where);
		}

		if(attrib){
			unsigned long al;

			if(attrib->bits.align){
				consty k;

				FOLD_EXPR(attrib->bits.align, stab);
				const_fold(attrib->bits.align, &k);

				if(k.type != CONST_NUM || !K_INTEGRAL(k.bits.num))
					die_at(&attrib->where, "aligned attribute not reducible to integer constant");
				al = k.bits.num.val.i;
			}else{
				al = platform_align_max();
			}

			max_al = fold_align(al, tal, max_al, &attrib->where);
			if(!d->bits.var.align)
				d->bits.var.align = umalloc(sizeof *d->bits.var.align);
		}

		d->bits.var.align->resolved = max_al;
	}

	if(is_static_duration && type_is_variably_modified(d->ref)){
		warn_at_print_error(
				&d->where,
				"static-duration variable length array");
		fold_had_error = 1;
		return;
	}

	if(d->bits.var.init.dinit){
		switch(d->store & STORE_MASK_STORE){
			case store_typedef:
				fold_had_error = 1;
				warn_at_print_error(&d->where, "initialised typedef");
				break;

			case store_extern:
				/* allow for globals - remove extern since it's a definition */
				if(stab->parent){
					die_at(&d->where, "externs can't be initialised");
				}else{
					warn_at(&d->where, "extern initialisation");
					d->store &= ~store_extern;
				}
				break;
		}

		/* don't generate for anonymous symbols
		 * they're done elsewhere (e.g. compound literals and strings)
		 */
		if(d->spel){
			/* this creates the below s->inits array */
			if(is_static_duration){
				fold_decl_global_init(d, stab);

			}else if(!d->bits.var.init.expr){
				decl_init_brace_up_fold(d, stab, /*struct_copy:*/1);

				decl_init_create_assignments_base(
						d->bits.var.init.dinit, d->ref,
						expr_set_where(
							expr_new_identifier(d->spel),
							&d->where),
						&d->bits.var.init.expr);

				fold_expr(d->bits.var.init.expr, stab);

			}else{
				ICE("fold_decl(%s) with no pinit_code?", d->spel);
			}
		}
	}
}

static void fold_decl_var_fieldwidth(decl *d, symtable *stab)
{
	consty k;

	FOLD_EXPR(d->bits.var.field_width, stab);
	const_fold(d->bits.var.field_width, &k);

	if(k.type != CONST_NUM)
		die_at(&d->where, "constant expression required for field width");
	if(K_FLOATING(k.bits.num))
		die_at(&d->where, "integral expression required for field width");

	if((sintegral_t)k.bits.num.val.i < 0)
		die_at(&d->where, "field width must be positive");

	if(k.bits.num.val.i == 0){
		/* allow anonymous 0-width bitfields
		 * we align the next bitfield to a boundary
		 */
		if(d->spel)
			die_at(&d->where,
					"none-anonymous bitfield \"%s\" with 0-width",
					d->spel);
	}else{
		const unsigned max = CHAR_BIT * type_size(d->ref, &d->where);
		if(k.bits.num.val.i > max){
			die_at(&d->where,
					"bitfield too large for \"%s\" (%u bits)",
					decl_to_str(d), max);
		}
	}

	if(!type_is_integral(d->ref))
		die_at(&d->where, "field width on non-integral field %s",
				decl_to_str(d));

	/* FIXME: only warn if "int" specified,
	 * i.e. detect explicit signed/unsigned */
	if(k.bits.num.val.i == 1 && type_is_signed(d->ref))
		warn_at(&d->where, "1-bit signed field \"%s\" takes values -1 and 0",
				decl_to_str(d));
}

void fold_decl(decl *d, symtable *stab)
{
	/* this is called from wherever we can define a
	 * struct/union/enum,
	 * e.g. a code-block (explicit or implicit),
	 *      global scope
	 * and an if/switch/while statement: if((struct A { int i; } *)0)...
	 * an argument list/type::func: f(struct A { int i, j; } *p, ...)
	 */
	int just_init = 0;
#define first_fold (!just_init)
	switch(d->fold_state){
		case DECL_FOLD_EXCEPT_INIT:
			just_init = 1;
		case DECL_FOLD_NO:
			break;
		case DECL_FOLD_INIT:
			return;
	}
	d->fold_state = DECL_FOLD_EXCEPT_INIT;

	if(first_fold){
		attribute *attr;

		fold_type_w_attr(d->ref, NULL, type_loc(d->ref), stab, d->attr);

		if(d->spel)
			fold_decl_add_sym(d, stab);

		if(((d->store & STORE_MASK_STORE) != store_typedef)
		/* __attribute__((weak)) is allowed on typedefs */
		&& (attr = attribute_present(d, attr_weak))
		&& decl_linkage(d) != linkage_external)
		{
			warn_at_print_error(&d->where,
					"weak attribute on declaration without external linkage");
			fold_had_error = 1;
		}
	}

	if(type_is(d->ref, type_func)){
		if(first_fold)
			fold_decl_func(d, stab);
	}else{
		if(first_fold && d->bits.var.field_width)
			fold_decl_var_fieldwidth(d, stab);

		if(d->fold_state == DECL_FOLD_EXCEPT_INIT){
			d->fold_state = DECL_FOLD_INIT;
			fold_decl_var(d, stab);
		}
	}

	/* name static decls */
	if(first_fold
	&& stab->parent
	&& (d->store & STORE_MASK_STORE) == store_static
	&& d->spel
	/* ignore static arguments - error handled elsewhere */
	&& (!d->sym || d->sym->type != sym_arg))
	{
		decl *in_fn = symtab_func(stab);
		UCC_ASSERT(in_fn, "not in function for \"%s\"", d->spel);

		d->spel_asm = out_label_static_local(
				in_fn->spel,
				d->spel);
	}
#undef first_fold
}

void fold_decl_global_init(decl *d, symtable *stab)
{
	expr *nonstd = NULL;
	const char *type;

	if(!type_is(d->ref, type_func) && !d->bits.var.init.dinit)
		return;

	/* this completes the array, if any */
	decl_init_brace_up_fold(d, stab, /*struct_copy:*/1);

	type = stab->parent ? "static" : "global";
	if(!decl_init_is_const(d->bits.var.init.dinit, stab, &nonstd)){
		warn_at_print_error(&d->bits.var.init.dinit->where,
				"%s %s initialiser not constant",
				type, decl_init_to_str(d->bits.var.init.dinit->type));

		fold_had_error = 1;
	}else if(nonstd){
		char wbuf[WHERE_BUF_SIZ];

		warn_at(&d->bits.var.init.dinit->where,
				"%s %s initialiser contains non-standard constant expression\n"
				"%s: note: %s expression here",
				type, decl_init_to_str(d->bits.var.init.dinit->type),
				where_str_r(wbuf, &nonstd->where),
				nonstd->f_str());
	}

}

static void warn_passable_func(decl *d)
{
	cc1_warn_at(&d->where, 0, WARN_RETURN_UNDEF,
			"control reaches end of non-void function %s",
			d->spel);
}

int fold_func_is_passable(decl *func_decl, type *func_ret, int warn)
{
	int passable = 0;
	struct
	{
		char *extra;
		where *where;
	} the_return = { NULL, NULL };

	if(attribute_present(func_decl, attr_noreturn)){
		if(!type_is_void(func_ret)){
			cc1_warn_at(&func_decl->where, 0, WARN_RETURN_UNDEF,
					"function \"%s\" marked no-return has a non-void return value",
					func_decl->spel);
		}


		if(fold_passable(func_decl->bits.func.code)){
			/* if we reach the end, it's bad */
			the_return.extra = "implicitly ";
			the_return.where = &func_decl->where;
			passable = 1;
		}else{
			stmt *ret = NULL;

			stmt_walk(func_decl->bits.func.code,
					stmt_walk_first_return, NULL, &ret);

			if(ret){
				/* obviously returns */
				the_return.extra = "";
				the_return.where = &ret->where;
			}
		}

		if(the_return.extra){
			cc1_warn_at(the_return.where, 0, WARN_RETURN_UNDEF,
					"function \"%s\" marked no-return %sreturns",
					func_decl->spel, the_return.extra);
		}

	}else if(!type_is_void(func_ret)){
		/* non-void func - check it doesn't return */
		if(fold_passable(func_decl->bits.func.code)){
			passable = 1;
			if(warn)
				warn_passable_func(func_decl);
		}
	}

	return passable;
}

void fold_func_code(stmt *code, where *w, char *sp, symtable *arg_symtab)
{
	decl **i;

	for(i = arg_symtab->decls; i && *i; i++){
		decl *d = *i;

		if(!d->spel)
			die_at(w, "argument %ld in \"%s\" is unnamed",
					i - arg_symtab->decls + 1, sp);

		if(!type_is_complete(d->ref))
			die_at(&d->where,
					"function argument \"%s\" has incomplete type '%s'",
					d->spel, type_to_str(d->ref));
	}

	fold_stmt(code);

	/* now decls are folded, layout both parameters and local variables */
	symtab_layout_decls(arg_symtab, 0);

	/* finally, check label coherence */
	symtab_chk_labels(symtab_func_root(arg_symtab));
}

void fold_global_func(decl *func_decl)
{
	if(func_decl->bits.func.code){
		symtable *const arg_symtab = DECL_FUNC_ARG_SYMTAB(func_decl);
		funcargs *args;
		type *func_ret = type_func_call(func_decl->ref, &args);

		if(type_is_tdef(func_decl->ref))
			warn_at(&func_decl->where,
					"typedef function implementation is an extension");

		fold_func_code(
				func_decl->bits.func.code,
				&func_decl->where,
				func_decl->spel,
				arg_symtab);

		if(fold_func_is_passable(func_decl, func_ret, 0)){
			if((fopt_mode & FOPT_FREESTANDING) == 0
			&& cc1_std >= STD_C99
			&& !strcmp(func_decl->spel, "main"))
			{
				/* hosted environment, in main. return 0 */
				stmt *zret = stmt_new_wrapper(return,
						func_decl->bits.func.code->symtab);

				zret->expr = expr_new_val(0);

				dynarray_add(&func_decl->bits.func.code->bits.code.stmts, zret);
				fold_stmt(zret);

			}else{
				warn_passable_func(func_decl);
			}
		}
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
			fold_had_error = 1;
			warn_at_print_error(&d->where,
					"invalid storage class '%s' on global scoped %s",
					decl_store_to_str(d->store),
					type_is(d->ref, type_func) ? "function" : "variable");
	}

	fold_decl(d, stab);

	if(type_is(d->ref, type_func))
		fold_global_func(d);
}

void fold_check_expr(expr *e, enum fold_chk chk, const char *desc)
{
	if(!e)
		return;

	UCC_ASSERT(e->tree_type, "no tree type");

	/* fatal ones first */

	if((chk & FOLD_CHK_ALLOW_VOID) == 0 && type_is_void(e->tree_type))
		die_at(&e->where, "%s requires non-void expression", desc);

	if(chk & FOLD_CHK_NO_ST_UN){
		struct_union_enum_st *sue;

		if((sue = type_is_s_or_u(e->tree_type))){
			die_at(&e->where, "%s involved in %s",
					sue_str(sue), desc);
		}
	}

	if(chk & FOLD_CHK_NO_BITFIELD){
		if(e && expr_kind(e, struct)){
			decl *d = e->bits.struct_mem.d;

			assert(!type_is(d->ref, type_func));

			if(d->bits.var.field_width)
				die_at(&e->where, "bitfield in %s", desc);
		}
	}

	if(chk & FOLD_CHK_INTEGRAL){
		if(type_is_floating(e->tree_type)){
			die_at(&e->where, "%s requires an integral expression (not \"%s\")",
					desc, type_to_str(e->tree_type));
		}
	}

	if(!(chk & FOLD_CHK_NOWARN_ASSIGN)
	&& !e->in_parens
	&& expr_kind(e, assign))
	{
		cc1_warn_at(&e->where, 0,WARN_TEST_ASSIGN, "assignment in %s", desc);
	}

	if(chk & FOLD_CHK_BOOL){
		if(!type_is_bool(e->tree_type)){
			cc1_warn_at(&e->where, 0, WARN_TEST_BOOL,
					"testing a non-boolean expression (%s), in %s",
					type_to_str(e->tree_type), desc);
		}
	}

	if(chk & FOLD_CHK_CONST_I){
		consty k;
		const_fold(e, &k);

		if(k.type != CONST_NUM || !K_INTEGRAL(k.bits.num))
			die_at(&e->where, "integral constant expected for %s", desc);
	}
}

void fold_stmt(stmt *t)
{
	UCC_ASSERT(t->symtab->parent, "symtab has no parent");

	t->f_fold(t);
}

void fold_funcargs(funcargs *fargs, symtable *stab, attribute *attr)
{
	attribute *da;
	unsigned long nonnulls = 0;

	/* check nonnull corresponds to a pointer arg */
	if((da = attr_present(attr, attr_nonnull)))
		nonnulls = da->bits.nonnull_args;

	if(fargs->arglist){
		/* check for unnamed params and extern/static specs */
		int i;

		for(i = 0; fargs->arglist[i]; i++){
			decl *const d = fargs->arglist[i];

			const int is_var = !type_is(d->ref, type_func);

			/* fold before for array checks, etc */
			if(is_var && d->bits.var.init.dinit)
				die_at(&d->where, "parameter '%s' is initialised", d->spel);
			fold_decl(d, stab);

			if(type_is_void(d->ref)){
				/* allow if it's the first, only and unnamed */
				if(i != 0 || fargs->arglist[1] || d->spel)
					die_at(&d->where, "function parameter is void");
			}

			/* convert any array definitions and functions to pointers */
			/* must be before the decl is folded (since fold checks this) */
			if(decl_conv_array_func_to_ptr(d)){
				/* refold if we converted */
				fold_type_w_attr(d->ref, NULL, type_loc(d->ref), stab, d->attr);
			}

			switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
				case store_auto:
				case store_static:
				case store_extern:
				case store_typedef:
				case store_inline:
					die_at(&fargs->where,
							"%s storage on \"%s\"",
							decl_store_to_str(d->store), d->spel);
					break;

				case store_register:
				case store_default:
					break;
			}

			/* ensure ptr, unless __attribute__((nonnull)) */
			if(nonnulls != ~0UL
			&& (nonnulls & (1 << i))
			&& !type_is(d->ref, type_ptr)
			&& !type_is(d->ref, type_block))
			{
				warn_at(&fargs->arglist[i]->where,
						"nonnull attribute applied to non-pointer argument '%s'",
						type_to_str(d->ref));
			}
		}

		if(i == 0 && nonnulls)
			warn_at(&fargs->where,
					"nonnull attribute applied to function with no arguments");
		else if(nonnulls != ~0UL && nonnulls & -(1 << i))
			warn_at(&fargs->where,
					"nonnull attributes above argument index %d ignored", i + 1);
	}else if(nonnulls){
		warn_at(&fargs->where, "nonnull attribute on parameterless function");
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

		/* functions are checked in fold_sym,
		 * where we compare local definitions too */
		if(d->proto_flag || !d->spel || !!type_is(d->ref, type_func))
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
			if(!type_is(d->ref, type_func) && d->bits.var.init.dinit){
				if(init){
					char wbuf[WHERE_BUF_SIZ];
					die_at(&init->where, "multiple definitions of \"%s\"\n"
							"%s: note: other definition here", init->spel,
							where_str_r(wbuf, &d->where));
				}
				init = d;
			}else if(type_is(d->ref, type_array) && type_is_complete(d->ref)){
				init = d;
				decl_default_init(d, stab);
			}
		}
		if(!init){
			/* no initialiser, give the last declared one a default init */
			d = globs[i];

			decl_default_init(d, stab);

			if(type_is(d->ref, type_array)){
				warn_at(&d->where,
						"tenative array definition assumed to have one element");
			}

			cc1_warn_at(&d->where, 0, WARN_TENATIVE_INIT,
					"default-initialising tenative definition of \"%s\"",
					d->spel);
		}
	}
}
