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
#include "fopt.h"
#include "cc1_target.h"

#include "ops/expr_cast.h"
#include "ops/expr_identifier.h"
#include "ops/expr_val.h"
#include "ops/expr_struct.h"
#include "ops/expr_assign.h"
#include "ops/expr_addr.h"

enum fold_type_chk
{
	FOLD_TYPE_NO_ARRAYQUAL = 1 << 0
};

int fold_had_error;
int fold_expect_error;

void fold_insert_casts(type *tlhs, expr **prhs, symtable *stab)
{
	/* insert a cast: rhs -> lhs */
	*prhs = expr_set_where(
			expr_new_cast(*prhs, tlhs, 1),
			&(*prhs)->where);

	/* need to fold the cast again - mainly for "loss of precision" warning */
	fold_expr_cast_descend(*prhs, stab, 0);
}

static int check_enum_cmp(
		type *lhs, type *rhs,
		where *w, const char *desc)
{
	struct_union_enum_st *sl, *sr;

	if(!(sl = type_is_enum(lhs)))
		return 0;
	if(!(sr = type_is_enum(rhs)))
		return 0;
	if(sl == sr)
		return 0;

	if(cc1_warn_at(w, enum_mismatch, "enum type mismatch in %s", desc)){
		note_at(w,
			"'enum %s' vs 'enum %s'",
			sl->spel, sr->spel);
	}

	return 1;
}

int fold_type_chk_warn(
		expr *maybe_lhs, type *tlhs, expr *rhs,
		int is_comparison,
		where *w, const char *desc)
{
	unsigned char *pwarn;
	type *const trhs = rhs->tree_type;
	int error = 1;
	const char *detail = "";
	const int allow_qual_subtraction = is_comparison;

	assert(!!maybe_lhs ^ !!tlhs);
	if(!tlhs)
		tlhs = maybe_lhs->tree_type;

	if(type_is_ptr_or_block(tlhs) && type_is_ptr_or_block(trhs)){
		if(is_comparison){
			pwarn = &cc1_warning.compare_distinct_pointer_types;
		}else{
			pwarn = &cc1_warning.incompatible_pointer_types;
		}
	}else{
		pwarn = &cc1_warning.mismatching_types;
	}

	switch(type_cmp(tlhs, trhs, TYPE_CMP_ALLOW_TENATIVE_ARRAY)){
		case TYPE_CONVERTIBLE_IMPLICIT:
			/* attempt to insert regardless, e.g. _Bool x = 5;
			 *  - they match but we need the _Bool cast */
			return 1;

		case TYPE_EQUAL:
			/* for enum types, we still want the cast, for warnings' sake */
			return check_enum_cmp(tlhs, trhs, w, desc);

		case TYPE_QUAL_ADD: /* const int <- int */
		case TYPE_QUAL_SUB: /* int <- const int */
		case TYPE_EQUAL_TYPEDEF:
			return 0;

		case TYPE_CONVERTIBLE_EXPLICIT:
		{
			int skip_warn = 0;

			error = 0;

			/* special case - allow pointer <--> 0-constant */
			if(type_is_ptr_or_block(tlhs)
			&& expr_is_null_ptr(rhs, NULL_STRICT_INT))
			{
				skip_warn = 1;
			}else if(maybe_lhs
			&& type_is_ptr_or_block(trhs)
			&& expr_is_null_ptr(maybe_lhs, NULL_STRICT_INT))
			{
				skip_warn = 1;
			}

			if(cc1_warning.null_zero_literal){
				skip_warn = 0;
				pwarn = &cc1_warning.null_zero_literal;
			}

			if(skip_warn){
				/* no warning, but still sign extend the zero */
				return 1;
			}

			goto warning;
		}

		case TYPE_QUAL_NESTED_CHANGE: /* char ** <- const char ** or vice versa */
			detail = "nested ";
			error = 0;
			goto warning;

		case TYPE_QUAL_POINTED_ADD:
			/* const char * <- char *
			 * okay for both comparisons and assignments */
			return 0;

		case TYPE_QUAL_POINTED_SUB:
			/* char * <- const char
			 * okay for comparisons, but not assignments */
			if(allow_qual_subtraction)
				return 0;
			error = 0;
			goto warning;

warning:
		case TYPE_NOT_EQUAL:
		{
			const char *fmt = "mismatching %stypes, %s";
			char buf[TYPE_STATIC_BUFSIZ];
			int show_note = 1;

			if(pwarn == &cc1_warning.compare_distinct_pointer_types){
				fmt = "distinct %spointer types in %s";
				detail = "";
			}

			if(error){
				warn_at_print_error(w, fmt, detail, desc);
				fold_had_error = 1;
			}else{
				show_note = cc1_warn_at_w(w, pwarn, fmt, detail, desc);
			}

			if(show_note){
				/* don't show line with this note */
				where loc = *w;
				loc.line_str = NULL;
				note_at(&loc, "'%s' vs '%s'", type_to_str_r(buf, tlhs), type_to_str(trhs));
			}

			if(error){
				return 0; /* don't cast - we don't want duplicate errors */
			}
			return 1; /* need cast */
		}
	}

	return 0;
}

static void fold_type_chk_and_cast_common(
		expr *lhs, type *tlhs, expr **prhs,
		symtable *stab, where *w,
		const char *desc)
{
	if(fold_type_chk_warn(lhs, tlhs, *prhs, /*is_comparison*/0, w, desc))
		fold_insert_casts(tlhs ? tlhs : lhs->tree_type, prhs, stab);
}

void fold_type_chk_and_cast(
		expr *lhs, expr **prhs,
		symtable *stab, where *w,
		const char *desc)
{
	fold_type_chk_and_cast_common(lhs, NULL, prhs, stab, w, desc);
}

void fold_type_chk_and_cast_ty(
		type *tlhs, expr **prhs,
		symtable *stab, where *w,
		const char *desc)
{
	fold_type_chk_and_cast_common(NULL, tlhs, prhs, stab, w, desc);
}

void fold_check_restrict(expr *lhs, expr *rhs, const char *desc, where *w)
{
	/* restrict operation checks */
	const enum type_qualifier ql = type_qual(lhs->tree_type),
	                          qr = type_qual(rhs->tree_type);

	if((ql & qual_restrict) && (qr & qual_restrict))
		cc1_warn_at(w, restrict_ptrs, "restrict pointers in %s", desc);
}

sym *fold_inc_writes_if_sym(expr *e, symtable *stab)
{
	sym *sym = expr_to_symref(e, stab);

	if(sym)
		sym->nwrites++;

	return sym;
}

void fold_expr_nodecay(expr *e, symtable *stab)
{
	if(e->tree_type)
		return;

	e->f_fold(e, stab);

	UCC_ASSERT(e->tree_type, "no tree_type after fold (%s)", e->f_str());
}

expr *fold_expr_lval2rval(expr *e, symtable *stab)
{
	/*
	 * C89:
	 *
	 * "Except when it is the operand of the sizeof operator ... an lvalue that
	 * has type "array of type" is converted to an expression that has type
	 * "pointer to type" that points to the initial member of the array object
	 * and is not an lvalue.
	 *
	 * C99 (6.3.2.1p3):
	 *
	 * "Except when it is the operand of the sizeof operator ... an expression
	 * that has type "array of type" is converted to an expression with type
	 * "pointer to type" that points to the initial element of the array object
	 * and is not an lvalue."
	 */
	int should_lval2rval, decayable;

	fold_expr_nodecay(e, stab);

	decayable = type_decayable(e->tree_type);
	should_lval2rval = (cc1_std >= STD_C99 && decayable);

	if(!should_lval2rval){
		switch(expr_is_lval(e)){
			case LVALUE_NO:
				break;

			case LVALUE_STRUCT:
				/* not a user-lvalue - only lval2rval if non-decayable type */
				should_lval2rval = !decayable;
				break;

			case LVALUE_USER_ASSIGNABLE:
				should_lval2rval = 1;
				break;
		}
	}

	if(should_lval2rval || type_is(e->tree_type, type_func)){
		struct_union_enum_st *enum_ty;
		expr *const orig_e = e;

		e = expr_set_where(
				expr_new_cast_lval_decay(e),
				&e->where);

		fold_expr_cast_descend(e, stab, 0);

		/* if we've just lval2rval'd a bitfield, we may need to promote it */
		if(expr_kind(orig_e, struct)
		&& orig_e->bits.struct_mem.d
		&& orig_e->bits.struct_mem.d->bits.var.field_width)
		{
			/*
			 * If an int can represent all values of the original type (as restricted
			 * by the width, for a bit-field), the value is converted to an int;
			 * otherwise, it is converted to an unsigned int.
			 */
			decl *bitfield = orig_e->bits.struct_mem.d;
			unsigned bf_nbits = const_fold_val_i(bitfield->bits.var.field_width);
			unsigned uint_nbits = type_primitive_size(type_int) * CHAR_BIT;
			unsigned sint_nbits = uint_nbits - 1; /* we assume 2's complement: */
			type *prom;

			if(bf_nbits <= sint_nbits){
				/* can be represented in int */
				if(!type_is_signed(bitfield->ref)){
					cc1_warn_at(&orig_e->where, bitfield_promotion,
							"promoting unsigned bitfield to int");
				}
				prom = type_nav_btype(cc1_type_nav, type_int);
			}else{
				/* must be represented in unsigned int */
				prom = type_nav_btype(cc1_type_nav, type_uint);
			}

			fold_insert_casts(prom, &e, stab);
		}

		if((enum_ty = type_is_enum(e->tree_type))){
			/* enums always become ints */
			fold_insert_casts(
					type_nav_int_enum(cc1_type_nav, enum_ty),
					&e, stab);
		}
	}

	return e;
}

expr *fold_expr_nonstructdecay(expr *e, symtable *stab)
{
	fold_expr_nodecay(e, stab);
	if(!type_is_s_or_u(e->tree_type))
		e = fold_expr_lval2rval(e, stab);
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
				if(platform_sys() == SYS_cygwin)
					def_conv = conv_x64_ms;
				else
					def_conv = conv_x64_sysv;
			}
		}

		conv = def_conv;
	}

	type_funcargs(r)->conv = conv;
}

void fold_check_embedded_flexar(
		struct_union_enum_st *sue, const where *loc, const char *desc)
{
	if(!sue || !sue->flexarr)
		return;

	cc1_warn_at(loc, flexarr_embed,
			"embedded flexible-array as %s", desc);
}

static void fold_type_w_attr(
		type *const r, type *const parent, const where *loc,
		symtable *stab, attribute **attr, const enum fold_type_chk chk)
{
	attribute **this_attr = NULL;
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
			if(parent
			&& (chk & FOLD_TYPE_NO_ARRAYQUAL)
			&& parent->type == type_cast)
			{
				fold_had_error = 1;
				warn_at_print_error(loc,
						"array has qualified type: %s",
						type_to_str(parent));
			}

			if(r->bits.array.size){
				consty k;
				where *array_loc = &r->bits.array.size->where;

				FOLD_EXPR(r->bits.array.size, stab);

				if(r->bits.array.vla_kind){
					if(cc1_std < STD_C99){
						cc1_warn_at(
								&r->bits.array.size->where,
								c89_vla,
								"variable length array is a C99 feature");

					}else{
						cc1_warn_at(&r->bits.array.size->where,
								vla, "use of variable length array");
					}

				}else{
					const_fold(r->bits.array.size, &k);

					UCC_ASSERT(k.type == CONST_NUM,
							"not a constant for array size");

					UCC_ASSERT(K_INTEGRAL(k.bits.num),
							"integral array should be checked during parse");

					/* allow zero length arrays */
					if(type_is_signed(r->bits.array.size->tree_type)
					&& (sintegral_t)k.bits.num.val.i < 0)
					{
						die_at(array_loc, "negative array size");
					}

					if(k.nonstandard_const){
						cc1_warn_at(&k.nonstandard_const->where,
								nonstd_arraysz,
								"%s-expr is a non-standard constant expression (for array size)",
								expr_str_friendly(k.nonstandard_const, 1));
					}
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
			break;

		case type_where:
			/* nothing to do */
			break;

		case type_cast:
			q_to_check = type_qual(r);
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

					/* 'above' may be null */
					if(above != sue){
						cc1_warn_at(&sue->where,
								private_struct,
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
			fold_expr_nodecay(p_expr, stab);

			if(r->bits.tdef.decl)
				fold_decl(r->bits.tdef.decl, stab);
			break;
		}
	}

	/*
	 * now we've folded, check for restrict
	 * since typedef int *intptr; intptr restrict a; is valid
	 */
	if(q_to_check & qual_restrict){
		if(!type_is_ptr(r) && !type_is(r, type_array)){
			cc1_warn_at(loc,
					bad_restrict,
					"restrict on non-pointer type '%s'",
					type_to_str(r));

		}else if(type_is_fptr(r->ref)){
			/* ^ next is a pointer, what's after that? */
			die_at(loc, "restrict qualified function pointer");
		}
	}

	fold_type_w_attr(r->ref, r,
			loc, stab, this_attr ? this_attr : attr,
			chk);

	/* checks that rely on r->ref being folded... */
	switch(r->type){
		case type_array:
			if(!type_is_complete(r->ref)){
				fold_had_error = 1;
				warn_at_print_error(loc,
						"array has incomplete type '%s'",
						type_to_str(r->ref));
			}
			fold_check_embedded_flexar(type_is_s_or_u(r->ref), loc, "array element");
			/* fall through to x()[] check */

		case type_func:
			if(type_is(r->ref, type_func)){
				fold_had_error = 1;
				warn_at_print_error(loc,
						r->type == type_func
							? "function returning a function"
							: "array of functions");
			}else if(r->type == type_func && type_is(r->ref, type_array)){
				fold_had_error = 1;
				warn_at_print_error(loc, "function returning an array");
			}
			break;

		case type_cast:
			if(type_is(r->ref, type_func)){
				/* C11 6.7.3.9
				 * If the specification of an array type includes any type qualifiers,
				 * the element type is so-qualified, not the array type. If the
				 * specification of a function type includes any type qualifiers, the
				 * behavior is undefined)
				 *
				 * Array types are handled by type_qualify()
				 */
				cc1_warn_at(loc, bad_funcqual, "qualifier on function type '%s'",
						type_to_str(r->ref));
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
}

void fold_type(type *t, symtable *stab)
{
	fold_type_w_attr(t, NULL, type_loc(t), stab, NULL, FOLD_TYPE_NO_ARRAYQUAL);
}

void fold_type_ondecl_w(decl *d, symtable *scope, where const *w, int is_arg)
{
	enum fold_type_chk fold_flags = 0;

	if(!is_arg)
		fold_flags |= FOLD_TYPE_NO_ARRAYQUAL;

	if(!w)
		w = &d->where;

	fold_type_w_attr(d->ref, NULL, w, scope, d->attr, fold_flags);
}

static void check_valid_align_within_min(int const al, int const min, where *w)
{
	/* allow zero */
	if(al & (al - 1)){
		warn_at_print_error(w, "alignment %d isn't a power of 2", al);
		fold_had_error = 1;
		return;
	}

	if(al == 0) /* 0 ignored as special case */
		return;

	if(al < min){
		/* gcc and clang allow this in some cases and will generate unaligned loads */
		warn_at_print_error(w, "can't reduce alignment (%d -> %d)", min, al);
		fold_had_error = 1;
	}
}

static void fold_ctor_dtor(
		decl *d, symtable *stab, enum attribute_type ty, const char *desc)
{
	attribute *attr;

	if(!(attr = attribute_present(d, ty)))
		return;

	if(attr->bits.priority){
		consty k;

		FOLD_EXPR(attr->bits.priority, stab);
		const_fold(attr->bits.priority, &k);

		if(!type_is_integral(attr->bits.priority->tree_type)
		|| k.type != CONST_NUM
		|| !K_INTEGRAL(k.bits.num))
		{
			warn_at_print_error(&attr->bits.priority->where,
					"%s priority not integral", desc);
			fold_had_error = 1;
		}
	}
}

static void fold_func_attr(decl *d, symtable *stab)
{
	funcargs *fa = type_funcargs(d->ref);
	attribute *da;

	if(attribute_present(d, attr_sentinel) && !fa->variadic)
		cc1_warn_at(&d->where, attr_sentinel_nonvariadic,
				"variadic function required for sentinel check");

	if((da = attribute_present(d, attr_format)))
		format_check_decl(d, da);

	if(type_is_void(type_called(d->ref, NULL))
	&& (da = attribute_present(d, attr_warn_unused)))
	{
		cc1_warn_at(&d->where, attr_unused_voidfn,
				"warn_unused attribute on function returning void");
	}

	fold_ctor_dtor(d, stab, attr_constructor, "constructor");
	fold_ctor_dtor(d, stab, attr_destructor, "destructor");

	/* copy calling convention from decl/type to funcargs */
	if((da = attribute_present(d, attr_call_conv))){
		assert(type_is_func_or_block(d->ref));

		/* this is safe - each function decl has its own funcargs */
		type_funcargs(d->ref)->conv = da->bits.conv;
	}
}

static void fold_check_enum_bitfield(
		struct_union_enum_st *en,
		unsigned bitwidth, decl *d)
{
	sue_member **i;

	for(i = en->members; i && *i; i++){
		enum_member *mem = (*i)->enum_member;

		integral_t val = const_fold_val_i(mem->val);

		if(integral_truncate_bits(val, bitwidth, NULL) != val)
			cc1_warn_at(&mem->where,
					overlarge_enumerator_bitfield,
					"enumerator %s (%lld) too large for its type (%s)",
					mem->spel, val, d->spel);
	}
}

void fold_decl_add_sym(decl *d, symtable *stab)
{
	/* must be before fold*, since sym lookups are done */
	if(d->sym){
		/* ignore */
	}else if(0 && d->proto){
		/* this links things too tightly and we end up with
		 * decls sharing funcargs (which are mutated, breaking
		 * the type_nav guarantees - easiest to not link here for now */
		decl *proto;

		for(proto = d; proto->proto; proto = proto->proto);

		d->sym = proto->sym;

	}else{
		enum sym_type ty;

		if(stab->are_params){
			ty = sym_arg;
		}else{
			/* no decl_store_duration_is_static() checks here:
			 * we haven't given it a sym yet */
			const int global = !stab->parent
				|| decl_store_static_or_extern(d->store)
				|| type_is(d->ref, type_func);

			ty = global ? sym_global : sym_local;
		}

		d->sym = sym_new(d, ty);
	}

	if(attribute_present(d, attr_cleanup)
	&& (d->store & STORE_MASK_STORE) != store_typedef
	&& (d->sym->type != sym_local || type_is(d->ref, type_func)))
	{
		cc1_warn_at(&d->where,
				attr_badcleanup,
				"cleanup attribute only applies to local variables");
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

	if(type_is_variably_modified(d->ref)){
		warn_at_print_error(&d->where, "function with variably modified type");
		fold_had_error = 1;
	}

	if(stab->parent){
		if(d->bits.func.code)
			die_at(&d->bits.func.code->where, "nested function %s", d->spel);
		else if((d->store & STORE_MASK_STORE) == store_static)
			die_at(&d->where, "block-scoped function cannot have static storage");
	}

	fold_func_attr(d, stab);
}

int fold_get_max_align_attribute(attribute **attribs, symtable *stab, const int min)
{
	int max = min;

	for(; attribs && *attribs; attribs++){
		attribute *attrib = *attribs;
		unsigned long al;
		consty k;

		if(attrib->type != attr_aligned || !attrib->bits.align)
			continue;

		FOLD_EXPR(attrib->bits.align, stab);
		const_fold(attrib->bits.align, &k);

		if(k.type != CONST_NUM || !K_INTEGRAL(k.bits.num))
			die_at(&attrib->where, "aligned attribute not reducible to integer constant");
		al = k.bits.num.val.i;

		check_valid_align_within_min(al, min, &attrib->where);
		if((int)al > max)
			max = al;
	}

	return max;
}

static int fold_decl_resolve_align(decl *d, symtable *stab, attribute *attrib)
{
	const unsigned min = type_align_no_attr(d->ref, &d->where);
	unsigned max = type_align(d->ref, &d->where); /* maybe with attr */

	struct decl_align *i;

	if((d->store & STORE_MASK_STORE) == store_register
	|| d->bits.var.field_width)
	{
		warn_at_print_error(&d->where, "can't align %s", decl_to_str(d));
		fold_had_error = 1;
		return max;
	}

	for(i = d->bits.var.align.first; i; i = i->next){
		unsigned al;

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

			fold_type_w_attr(ty, NULL, type_loc(d->ref),
					stab, d->attr, FOLD_TYPE_NO_ARRAYQUAL);

			al = type_align(ty, &d->where);
		}

		check_valid_align_within_min(al, min, &attrib->where);
		if(al > max)
			max = al;
	}

	if(attrib){
		attribute *stash[2];
		unsigned attrib_max;

		stash[0] = attrib;
		stash[1] = NULL;
		attrib_max = fold_get_max_align_attribute(stash, stab, min);

		if(attrib_max > max)
			max = attrib_max;
	}

	return max;
}

static void fold_decl_var_align(decl *d, symtable *stab)
{
	attribute *attrib;

	if(!d->spel){
		/* we ignore these, alignment only applies to decls, not types
		 * i.e.
		 * __attribute((aligned(..))) enum E { ... };
		 * ^~~~~~~~~~~~~~~~~~~~~~~~~~ ignored
		 */
		return;
	}

	attrib = attribute_present(d, attr_aligned);

	if(d->bits.var.align.first || attrib){
		d->bits.var.align.resolved = fold_decl_resolve_align(d, stab, attrib);
	}
}

static void fold_decl_var_vm(decl *d, symtable *stab, int const is_static_duration)
{
	int vla;
	if((is_static_duration || (d->store & STORE_MASK_STORE) == store_extern)
	&& type_is_variably_modified_vla(d->ref, &vla))
	{
		/* allow static VMs in local scope */
		int ok = (d->store & STORE_MASK_STORE) == store_static
			&& !vla && stab->parent;

		if(!ok){
			warn_at_print_error(
					&d->where,
					"%s variabl%s",
					is_static_duration ? "static-duration" : "extern-linkage",
					vla ? "e length array" : "y modified type");
			fold_had_error = 1;
			return;
		}
	}
}

static void fold_decl_var_dinit(
		decl *d, symtable *stab, int const is_static_duration)
{
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
					cc1_warn_at(&d->where, extern_init, "extern initialisation");
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
				decl_init_brace_up_fold(d, stab);

				decl_init_create_assignments_base_and_fold(
						d,
						expr_set_where(
							expr_new_identifier(d->spel),
							&d->where),
						stab);

			}else{
				ICE("fold_decl(%s) with no pinit_code?", d->spel);
			}
		}
	}
}

static void fold_decl_var(decl *d, symtable *stab)
{
	int is_static_duration = !stab->parent
		|| (d->store & STORE_MASK_STORE) == store_static;

	if((d->store & STORE_MASK_EXTRA) == store_inline){
		warn_at_print_error(&d->where, "inline on non-function");
		fold_had_error = 1;
	}

	fold_decl_var_vm(d, stab, is_static_duration);
	fold_decl_var_dinit(d, stab, is_static_duration);
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
		cc1_warn_at(&d->where, bitfield_onebit_int,
				"1-bit signed field \"%s\" takes values -1 and 0",
				decl_to_str(d));

	{
		struct_union_enum_st *e;
		if((e = type_is_enum(d->ref)))
			fold_check_enum_bitfield(e, k.bits.num.val.i, d);
	}
}

static void fold_decl_check_ctor_dtor(decl *d, symtable *stab)
{
	attribute *ctor, *dtor;
	const char *name;
	where *w;

	ctor = attribute_present(d, attr_constructor);
	dtor = attribute_present(d, attr_destructor);

	if(!ctor && !dtor)
		return;

	decl_use(d);
	w = &(ctor ? ctor : dtor)->where;
	name = ctor ? "constructor" : "destructor";

	if(!type_is(d->ref, type_func)){
		cc1_warn_at(w, attr_ctor_dtor_bad, "%s attribute on non-function", name);
		return;
	}

	if(stab->parent){
		cc1_warn_at(w, attr_ctor_dtor_bad, "%s attribute on non-global function", name);
		return;
	}
}

static void fold_decl_check_section_alias(decl *d, const int su_member)
{
	attribute *section;
	attribute *alias;

	section = attribute_present(d, attr_section);
	alias = attribute_present(d, attr_alias);

	if(su_member){
		if(section)
			cc1_warn_at(&section->where, attr_ignored, "section attribute on member");
		if(alias)
			cc1_warn_at(&alias->where, attr_ignored, "alias attribute on member");

		if(alias || section)
			return;
	}

	if(alias){
		decl *target = alias->bits.alias;

		if(!type_is(d->ref, type_func) && !cc1_target_details.alias_variables){
			warn_at_print_error(&d->where, "__attribute__((alias(...))) not supported on this target (for variables)");
			fold_had_error = 1;
		}

		if(decl_defined(d, 0)){
			warn_at_print_error(&d->where, "alias \"%s\" cannot be a definition", d->spel);
			fold_had_error = 1;

		}else if(!decl_defined(target, 0)){
			warn_at_print_error(&alias->where, "target \"%s\" of alias \"%s\" isn't a definition", target->spel, d->spel);
			fold_had_error = 1;
		}
	}

	if(alias && section){
		const char *this_section = section->bits.section;

		attribute *alias_section = attribute_present(alias->bits.alias, attr_section);
		const char *target_section = alias_section
			? alias_section->bits.section
			: NULL;

		if(!target_section || strcmp(target_section, this_section)){
			warn_at_print_error(&section->where,
					"alias and target have different sections");
			fold_had_error = 1;
		}
	}
}

static void fold_decl_attrs(decl *d, symtable *stab)
{
	attribute *attr;

	if(((d->store & STORE_MASK_STORE) != store_typedef)
	/* __attribute__((weak)) is allowed on typedefs */
	&& (attr = attribute_present(d, attr_weak))
	&& decl_linkage(d) != linkage_external)
	{
		warn_at_print_error(&d->where,
				"weak attribute on declaration without external linkage");
		fold_had_error = 1;
	}

	fold_decl_check_ctor_dtor(d, stab);
}

void fold_decl_attrs_requiring_fnbody(decl *d, const int su_member)
{
	fold_decl_check_section_alias(d, su_member);
}

void fold_decl_maybe_member(decl *d, symtable *stab, int su_member)
{
	/* this is called from wherever we can define a
	 * struct/union/enum,
	 * e.g. a code-block (explicit or implicit),
	 *      global scope
	 * and an if/switch/while statement: if((struct A { int i; } *)0)...
	 * an argument list/type::func: f(struct A { int i, j; } *p, ...)
	 */
	int just_init = 0;
	int first_fold;

	switch(d->fold_state){
		case DECL_FOLD_EXCEPT_INIT:
			just_init = 1;
		case DECL_FOLD_NO:
			break;
		case DECL_FOLD_INIT:
			return;
	}
	d->fold_state = DECL_FOLD_EXCEPT_INIT;

	first_fold = !just_init;

	if(first_fold){
		fold_type_w_attr(d->ref, NULL, type_loc(d->ref),
				stab, d->attr, FOLD_TYPE_NO_ARRAYQUAL);

		if(!su_member
		&& (d->spel || stab->are_params)
		&& (!STORE_IS_TYPEDEF(d->store) || type_is_variably_modified(d->ref)))
		{
			fold_decl_add_sym(d, stab);
		}

		fold_decl_attrs(d, stab);
	}

	/* name static decls - do this before handling init,
	 * in case the decl is used in its own init */
	if(first_fold
	&& stab->parent
	&& (d->store & STORE_MASK_STORE) == store_static
	&& d->spel
	/* ignore static arguments - error handled elsewhere */
	&& (!d->sym || d->sym->type != sym_arg))
	{
		decl *in_fn = symtab_func(stab);
		UCC_ASSERT(in_fn, "not in function for \"%s\"", d->spel);

		assert(!d->spel_asm);
		d->spel_asm = out_label_static_local(
				in_fn->spel,
				d->spel);
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

	if(first_fold)
		fold_decl_var_align(d, stab);
}

void fold_decl(decl *d, symtable *stab)
{
	fold_decl_maybe_member(d, stab, 0);
}

void fold_check_decl_complete(decl *d)
{
	if(!d->spel)
		return;

	switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
		case store_typedef:
		case store_extern:
			return;
		case store_static:
		case store_register:
		case store_default:
		case store_auto:
			break;
		case store_inline:
			assert(0);
	}

	if(!type_is_complete(d->ref)){
		struct_union_enum_st *sue = type_is_s_or_u_or_e(d->ref);

		warn_at_print_error(&d->where, "\"%s\" has incomplete type '%s'",
				d->spel, type_to_str(d->ref));
		fold_had_error = 1;

		if(sue)
			note_at(&sue->where, "forward declared here");
	}
}

void fold_decl_global_init(decl *d, symtable *stab)
{
	expr *nonstd = NULL, *nonconst = NULL;
	const char *type;

	if(!type_is(d->ref, type_func) && !d->bits.var.init.dinit)
		return;

	/* this completes the array, if any */
	decl_init_brace_up_fold(d, stab);

	type = stab->parent ? "static" : "global";
	if(!decl_init_is_const(d->bits.var.init.dinit, stab, d->ref, &nonstd, &nonconst)){
		warn_at_print_error(&d->bits.var.init.dinit->where,
				"%s %s initialiser not constant",
				type, decl_init_to_str(d->bits.var.init.dinit->type), nonconst);

		fold_had_error = 1;

		if(nonconst){
			note_at(&nonconst->where,
				"first non-constant expression here (%s)",
				nonconst->f_str());
		}
	}else if(nonstd){
		if(cc1_warn_at(&d->bits.var.init.dinit->where,
				nonstd_init,
				"%s %s initialiser contains non-standard constant expression",
				type, decl_init_to_str(d->bits.var.init.dinit->type)))
		{
			note_at(&nonstd->where, "%s expression here", expr_str_friendly(nonstd, 1));
		}
	}
}

static void warn_passable_func(decl *d)
{
	cc1_warn_at(&d->where, return_undef,
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
			cc1_warn_at(&func_decl->where, return_undef,
					"function \"%s\" marked no-return has a non-void return value",
					func_decl->spel);
		}


		if(fold_passable(func_decl->bits.func.code, 0)){
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
			cc1_warn_at(the_return.where, return_undef,
					"function \"%s\" marked no-return %sreturns",
					func_decl->spel, the_return.extra);
		}

	}else if(!type_is_void(func_ret)){
		/* non-void func - check it doesn't return */
		if(fold_passable(func_decl->bits.func.code, 0)){
			passable = 1;
			if(warn)
				warn_passable_func(func_decl);
		}
	}

	return passable;
}

void fold_func_code(stmt *code, where *w, char *sp, symtable *arg_symtab)
{
	decl **i, **const start = symtab_decls(arg_symtab);

	for(i = start; i && *i; i++){
		decl *d = *i;

		if(!d->spel){
			if(cc1_std < STD_C2X){
				warn_at_print_error(w,
					"argument %ld in \"%s\" is unnamed",
					i - start + 1, sp);

				fold_had_error = 1;
			}
		}

		if(!type_is_complete(d->ref))
			die_at(&d->where,
					"function argument \"%s\" has incomplete type '%s'",
					d->spel, type_to_str(d->ref));
	}

	fold_stmt(code);

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
			cc1_warn_at(&func_decl->where,
					typedef_fnimpl,
					"typedef function implementation is an extension");

		if(type_is_s_or_u(func_ret))
			cc1_warn_at(&func_decl->where,
					aggregate_return,
					"function returns aggregate (%s)",
					type_to_str(func_ret));

		if(!type_is_void(func_ret) && !type_is_complete(func_ret)){
			warn_at_print_error(&func_decl->where, "incomplete return type");
			fold_had_error = 1;
		}

		fold_func_code(
				func_decl->bits.func.code,
				&func_decl->where,
				func_decl->spel,
				arg_symtab);

		if(fold_func_is_passable(func_decl, func_ret, 0)){
			if(cc1_std >= STD_C99 && DECL_IS_HOSTED_MAIN(func_decl)){
				/* hosted environment, in main. return 0 */
				stmt *zret = stmt_new_wrapper(return,
						func_decl->bits.func.code->symtab);

				zret->expr = expr_set_where(expr_new_val(0), &zret->where);

				dynarray_add(&func_decl->bits.func.code->bits.code.stmts, zret);
				fold_stmt(zret);

			}else if(!type_is_void(func_ret)){
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

int fold_check_expr(const expr *e, enum fold_chk chk, const char *desc)
{
	if(!e)
		return 0;

	UCC_ASSERT(e->tree_type, "no tree type");

	/* fatal ones first */

	if((chk & FOLD_CHK_ALLOW_VOID) == 0 && type_is_void(e->tree_type)){
		warn_at_print_error(&e->where, "%s requires non-void expression", desc);
		fold_had_error = 1;
		return 1;
	}

	if(chk & FOLD_CHK_NO_ST_UN){
		struct_union_enum_st *sue;

		if((sue = type_is_s_or_u(e->tree_type))){
			warn_at_print_error(&e->where, "%s involved in %s",
					sue_str(sue), desc);
			fold_had_error = 1;
			return 1;
		}
	}

	if(chk & FOLD_CHK_NO_BITFIELD && expr_is_struct_bitfield(e)){
		warn_at_print_error(&e->where, "bitfield in %s", desc);
		fold_had_error = 1;
		return 1;
	}

	if(chk & (FOLD_CHK_ARITHMETIC | FOLD_CHK_INTEGRAL)){
		int (*chkfn)(type *) = chk & FOLD_CHK_ARITHMETIC
				? type_is_arith : type_is_integral;

		if(!chkfn(e->tree_type)){
			fold_had_error = 1;
			warn_at_print_error(&e->where,
					"%s requires an %s type (not \"%s\")",
					desc,
					chk & FOLD_CHK_ARITHMETIC
						? "arithmetic"
						: "integral",
					type_to_str(e->tree_type));
			return 1;
		}
	}

	if(!(chk & FOLD_CHK_NOWARN_ASSIGN)
	&& !e->in_parens
	&& expr_kind(e, assign))
	{
		cc1_warn_at(&e->where,test_assign, "assignment in %s", desc);
	}

	if(chk & FOLD_CHK_BOOL){
		switch(type_bool_category(e->tree_type)){
			case TYPE_BOOLISH_OK:
				break;

			case TYPE_BOOLISH_CONV:
				cc1_warn_at(&e->where, test_bool,
						"testing a non-boolean expression (%s), in %s",
						type_to_str(e->tree_type), desc);
				break;

			case TYPE_BOOLISH_NO:
				fold_had_error = 1;
				warn_at_print_error(&e->where,
						"%s-condition requires scalar type (not '%s')",
						desc, type_to_str(e->tree_type));
				break;
		}

		if(expr_kind(e, addr)){
			expr *addr_of = expr_addr_target(e);

			if(addr_of && expr_is_lval(addr_of) == LVALUE_USER_ASSIGNABLE){
				decl *d = expr_to_declref(addr_of, NULL);

				if(!d || !attribute_present(d, attr_weak)){
					cc1_warn_at(&e->where, address_of_lvalue,
							"address of lvalue (%s) is always true",
							type_to_str(addr_of->tree_type));
				}
			}
		}

		/* if we're in a nonnull function and this expr is a nonnull expr,
		 * warn about testing a nonnull expr */
		if(expr_attr_present(REMOVE_CONST(expr *, e), attr_nonnull)){
			/* this works because nonnull can be on the function type,
			 * not just the decl, but those attributes are propagated
			 * to the parameters */
			cc1_warn_at(&e->where, attr_nonnull_tested, "testing a nonnull value");
		}
	}

	if(chk & FOLD_CHK_CONST_I){
		consty k;
		const_fold((expr *)e, &k);

		if(k.type != CONST_NUM || !K_INTEGRAL(k.bits.num)){
			warn_at_print_error(&e->where, "integral constant expected for %s", desc);
			fold_had_error = 1;
			return 1;
		}
	}

	return 0;
}

void fold_stmt(stmt *t)
{
	UCC_ASSERT(t->symtab->parent, "symtab has no parent");

	t->f_fold(t);
}

void fold_funcargs(funcargs *fargs, symtable *stab, attribute **attr)
{
	attribute *nonnull_attr;
	unsigned long nonnulls = 0;

	/* check nonnull corresponds to a pointer arg */
	if((nonnull_attr = attr_present(attr, attr_nonnull)))
		nonnulls = nonnull_attr->bits.nonnull_args;

	if(fargs->arglist){
		/* check for unnamed params and extern/static specs */
		int i;
		int seen_ptr = 0;

		for(i = 0; fargs->arglist[i]; i++){
			decl *const d = fargs->arglist[i];

			const int is_var = !type_is(d->ref, type_func);
			int is_ptr;

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
				fold_type_w_attr(d->ref, NULL, type_loc(d->ref),
						stab, d->attr, FOLD_TYPE_NO_ARRAYQUAL);
			}

			switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
				case store_auto:
				case store_static:
				case store_extern:
				case store_typedef:
				case store_inline:
				{
					const char *spel = d->spel, *quote = "\"";

					if(!spel){
						spel = "unnamed argument";
						quote = "";
					}

					warn_at_print_error(&d->where,
							"%s storage on %s%s%s",
							decl_store_to_str(d->store),
							quote, spel, quote);

					fold_had_error = 1;
					continue;
				}

				case store_register:
				case store_default:
					break;
			}

			is_ptr = type_is(d->ref, type_ptr) || type_is(d->ref, type_block);

			/* ensure ptr, unless __attribute__((nonnull)) */
			if(nonnulls & (1 << i)){
				attribute **nonnull_attr;

				if(nonnulls != ~0UL && !is_ptr){
					cc1_warn_at(&fargs->arglist[i]->where,
							attr_nonnull_nonptr,
							"nonnull attribute applied to non-pointer argument '%s'",
							type_to_str(d->ref));
				}

				nonnull_attr = umalloc(2 * sizeof(*nonnull_attr));
				nonnull_attr[0] = attribute_new(attr_nonnull);

				/* apply nonnull attribute to decl type regardless */
				d->ref = type_attributed(d->ref, nonnull_attr);
			}

			seen_ptr |= is_ptr;
		}

		if(i == 0 && nonnulls)
			cc1_warn_at(&fargs->where,
					attr_nonnull_noargs,
					"nonnull attribute applied to function with no arguments");
		else if(nonnulls != ~0UL && nonnulls & -(1 << i))
			cc1_warn_at(&fargs->where,
					attr_nonnull_oob,
					"nonnull attributes above argument index %d ignored", i + 1);
		else if(!seen_ptr && nonnulls)
			cc1_warn_at(&fargs->where,
					attr_nonnull_noptrs,
					"nonnull attributes applied to function with no pointer arguments");

	}else if(nonnulls){
		cc1_warn_at(&fargs->where,
				attr_nonnull_noargs,
				"nonnull attribute on parameterless function");
	}
}

int fold_passable_yes(stmt *s, int break_means_passable)
{ (void)s; (void)break_means_passable; return 1; }

int fold_passable_no(stmt *s, int break_means_passable)
{ (void)s; (void)break_means_passable; return 0; }

int fold_passable(stmt *s, int break_means_passable)
{
	return s->f_passable(s, break_means_passable);
}

void fold_merge_tenatives(symtable *stab)
{
	decl **const globs = symtab_decls(stab);

	int i;
	/* go in reverse - check the last declared decl so we
	 * can go up the proto-chain */
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
			/* look for an explicit init */
			if(!type_is(d->ref, type_func) && d->bits.var.init.dinit){
				if(init){
					warn_at_print_error(&init->where, "multiple definitions of \"%s\"", init->spel);
					note_at(&d->where, "other definition here");
					fold_had_error = 1;
				}
				init = d;
			}else if(!init /* no explicit init - complete array? */
			&& type_is(d->ref, type_array)
			&& type_is_complete(d->ref))
			{
				init = d;
				decl_default_init(d, stab);
			}
		}
		if(!init){
			/* no initialiser, give the last declared one a default init */
			d = globs[i];

			decl_default_init(d, stab);

			if(type_is(d->ref, type_array)){
				cc1_warn_at(&d->where,
						tenative_array_1elem,
						"tenative array definition assumed to have one element");
			}

			cc1_warn_at(&d->where, tenative_init,
					"default-initialising tenative definition of \"%s\"",
					d->spel);
		}
	}
}
