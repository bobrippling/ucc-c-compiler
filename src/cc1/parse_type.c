#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "decl_init.h"
#include "funcargs.h"

#include "tokenise.h"
#include "tokconv.h"
#include "str.h"

#include "sue.h"
#include "sym.h"

#include "cc1.h"
#include "cc1_where.h"
#include "fopt.h"

#include "parse_type.h"
#include "parse_expr.h"
#include "parse_attr.h"
#include "parse_init.h"
#include "parse_stmt.h"

#include "expr.h"
#include "type_nav.h"
#include "type_is.h"

#include "fold.h"
#include "fold_sue.h"

#include "ops/expr_sizeof.h"

static decl *parse_decl_stored_aligned(
		type *btype, enum decl_mode mode,
		enum decl_storage store, struct decl_align *align,
		symtable *scope, symtable *add_to_scope,
		int is_arg);

static type *default_type(void);
static int parse_at_decl_spec(void);

static int can_complete_existing_sue(
		struct_union_enum_st *sue, enum type_primitive new_tag)
{
	return sue->primitive == new_tag && !sue->got_membs;
}

static void emit_redef_sue_error(
		where const *new_sue_loc,
		struct_union_enum_st *already_existing,
		enum type_primitive prim,
		const int is_definition)
{
	const int equal_tags = (prim == already_existing->primitive);

	fold_had_error = 1;

	warn_at_print_error(new_sue_loc,
			"rede%s of %s%s%s",
			is_definition ? "finition" : "claration",
			sue_str(already_existing),
			equal_tags ? " in scope" : " as ",
			equal_tags ? "" : type_primitive_to_str(prim));

	note_at(&already_existing->where, "previous definition here");
}

static struct_union_enum_st *parse_sue_definition(
		sue_member ***members,
		char **const spel,
		enum type_primitive prim,
		symtable *scope,
		where *const sue_loc)
{
	struct_union_enum_st *predecl_sue = NULL;

	if(*spel){
		/* predeclare so the struct/union can be recursive.
		 * we know this is fine to explicitly declare as we're
		 * parsing a definition, so we aren't worried about
		 * referring to a sue in an outer scope
		 */
		struct_union_enum_st *already_existing = sue_find_this_scope(scope, *spel);

		if(already_existing){
			if(can_complete_existing_sue(already_existing, prim)){
				predecl_sue = already_existing;
			}else{
				emit_redef_sue_error(sue_loc, already_existing, prim, /*isdef:*/1);

				free(*spel), *spel = NULL;
			}
		}else{
			predecl_sue = sue_predeclare(
					scope, ustrdup(*spel),
					prim, sue_loc);
		}
	}

	/* sue is now in scope, but incomplete */
	if(prim == type_enum){
		if(!predecl_sue){
			assert(!*spel);
			predecl_sue = sue_predeclare(scope, NULL, prim, sue_loc);
		}

		for(;;){
			where w;
			expr *e;
			char *sp;
			attribute **en_attr = NULL;

			where_cc1_current(&w);
			sp = token_current_spel();
			EAT(token_identifier);

			parse_add_attr(&en_attr, scope);

			if(accept(token_assign)){
				e = PARSE_EXPR_CONSTANT(scope, 0); /* no commas */
				/* ensure we fold before this enum member is added to the scope,
				 * e.g.
				 * enum { A };
				 * f()
				 * {
				 *   enum {
				 *     A = A // the right-most A here resolves to the global A
				 *   };
				 * }
				 */

				fold_expr_nodecay(e, scope);
			}else{
				e = NULL;
			}

			enum_vals_add(members, &w, sp, e, /*released:*/en_attr);

			predecl_sue->members = *members;

			if(!accept_where(token_comma, &w))
				break;

			if(curtok != token_identifier){
				if(cc1_std < STD_C99)
					cc1_warn_at(&w, c89_parse_trailingcomma,
							"trailing comma in enum definition");
				break;
			}
		}

		predecl_sue->members = NULL;

	}else{
		/* always allow nameless structs (C11)
		 * we don't allow tagged ones unless
		 * -fms-extensions or -fplan9-extensions
		 */
		decl **dmembers = NULL;
		decl **i;

		while(parse_decl_group(
					DECL_MULTI_CAN_DEFAULT
					| DECL_MULTI_ACCEPT_FIELD_WIDTH
					| DECL_MULTI_NAMELESS
					| DECL_MULTI_IS_STRUCT_UN_MEMB
					| DECL_MULTI_ALLOW_ALIGNAS,
					/*newdecl_context:*/0,
					scope, NULL, &dmembers))
		{
		}

		if(dmembers){
			for(i = dmembers; *i; i++)
				dynarray_add(members, sue_member_from_decl(*i));

			dynarray_free(decl **, dmembers, NULL);
		}

		sue_member_init_dup_check(*members);
	}

	return predecl_sue;
}

static type *parse_sue_finish(
		struct_union_enum_st *predecl_sue,
		sue_member **members,
		int const got_membs,
		char *spel,
		enum type_primitive const prim,
		attribute ***this_sue_attr,
		attribute ***this_type_attr,
		int const already_exists,
		symtable *const scope,
		where *const sue_loc)
{
	struct_union_enum_st *sue = predecl_sue;

	if(!sue){
		assert(!spel);
		sue = sue_predeclare(scope, NULL, prim, sue_loc);
	}

	if(got_membs)
		sue_define(sue, members);

	/* sue may already exist */
	if(*this_sue_attr){
		if(already_exists){
			cc1_warn_at(&(*this_sue_attr)[0]->where,
					ignored_attribute,
					"cannot add attributes to already-defined type");
		}else{
			dynarray_add_array(&sue->attr, attribute_array_retain(*this_sue_attr));
		}
	}

	fold_sue(sue, scope);

	return type_attributed(
			type_nav_suetype(cc1_type_nav, sue),
			*this_type_attr);
}

static int parse_token_creates_sue(enum token tok)
{
	/*
	 * // new type:
	 * struct A;
	 *
	 * // reference to existing:
	 * struct A *p;
	 * struct A a;
	 * struct A (x);
	 * struct A ()
	 * f(struct A);
	 */
	switch(tok){
		case token_semicolon:
			return 1;

		case token_close_paren:
		case token_comma:
			return 0;

		default:
			return !parse_at_decl_spec();
	}
}

/* newdecl_context:
 * struct B { int b; };
 * {
 *   struct A { struct B; }; // this is not a new B
 * };
 */
static type *parse_type_sue(
		enum type_primitive const prim,
		symtable *const scope,
		int const newdecl_context)
{
	type *retty;
	int is_definition = 0;
	int already_exists = 0;
	char *spel = NULL;
	struct_union_enum_st *predecl_sue = NULL;
	sue_member **members = NULL;
	attribute **this_sue_attr = NULL, **this_type_attr = NULL;
	where sue_loc;

	/* location is the tag, by default */
	where_cc1_current(&sue_loc);

	/* struct __attr__(()) name { ... } ... */
	parse_add_attr(&this_sue_attr, scope);

	if(curtok == token_identifier){
		/* update location to be the name, since we have one */
		where_cc1_current(&sue_loc);

		spel = token_current_spel();
		EAT(token_identifier);
		where_cc1_adj_identifier(&sue_loc, spel);
	}

	if(accept(token_open_block)){
		is_definition = 1;

		predecl_sue = parse_sue_definition(
				&members, &spel, prim, scope, &sue_loc);

		EAT(token_close_block);

	}else{
		int descended;

		if(!spel){
			fold_had_error = 1;

			warn_at_print_error(
					NULL,
					"expected: %s definition or name",
					sue_str_type(prim));

			attribute_array_release(&this_sue_attr);
			free(spel);
			return type_nav_btype(cc1_type_nav, type_int);

		}

		predecl_sue = sue_find_descend(scope, spel, &descended);

		if(predecl_sue){
			/* if we have found a s/u/e BUT we descended in scope,
			 * then we're actually declaring a new one, if we're at
			 * a semi-colon.
			 * i.e.
			 * struct A { ... };
			 * void f()
			 * {
			 *     struct A; // a new type
			 * }
			 *
			 * provided we're in a newdecl context. i.e. the following
			 * is not a new type declaration
			 *
			 * struct A { ... };
			 * void f()
			 * {
			 *     struct irrelevant_name
			 *     {
			 *         struct A; // a reference to the outside type
			 *     };
			 * }
			 */
			int redecl_error = 0;
			const int prim_mismatch = (predecl_sue->primitive != prim);

			if(!descended && prim_mismatch)
				redecl_error = 1;
			else if(prim_mismatch && !parse_token_creates_sue(curtok))
					redecl_error = 1;

			if(redecl_error){
				emit_redef_sue_error(
						&sue_loc,
						predecl_sue,
						prim,
						/*isdef:*/is_definition);
			}

			if(descended && newdecl_context && parse_token_creates_sue(curtok))
				predecl_sue = NULL;

			already_exists = !!predecl_sue;
		}

		if(!predecl_sue){
			/* forward definition */
			predecl_sue = sue_predeclare(
					scope, spel, prim,
					&sue_loc);

			if(prim == type_enum){
				cc1_warn_at(&predecl_sue->where, predecl_enum,
						"forward-declaration of enum %s", predecl_sue->spel);
			}
		}
	}

	/* struct A { ... } __attr__
	 *
	 * - struct is incomplete before this point, so we handle the
	 *   attributes before sue_decl()
	 */
	parse_add_attr(&this_type_attr, scope);

	retty = parse_sue_finish(
			predecl_sue, members, is_definition,
			spel, prim,
			&this_sue_attr,
			&this_type_attr,
			already_exists,
			scope, &sue_loc);

	attribute_array_release(&this_sue_attr);
	attribute_array_release(&this_type_attr);

	return retty;
}

static void parse_add_attr_out(
		attribute ***append, symtable *scope, int *got_kw)
{
	if(got_kw)
		*got_kw = 0;

	while(accept(token_attribute)){
		if(got_kw)
			*got_kw = 1;

		cc1_warn_at(NULL, gnu_attribute, "use of GNU __attribute__");
		EAT(token_open_paren);
		EAT(token_open_paren);

		if(curtok != token_close_paren)
			dynarray_add_tmparray(append, parse_attr(scope));

		EAT(token_close_paren);
		EAT(token_close_paren);
	}
}

void parse_add_attr(attribute ***append, symtable *scope)
{
	parse_add_attr_out(append, scope, NULL);
}

static decl *parse_at_tdef(symtable *scope)
{
	struct symtab_entry ent;

	if(curtok == token_identifier
	&& symtab_search(scope, token_current_spel_peek(), NULL, &ent)
	&& ent.type == SYMTAB_ENT_DECL
	&& STORE_IS_TYPEDEF(ent.bits.decl->store))
	{
		return ent.bits.decl;
	}

	return NULL;
}

int parse_at_decl(symtable *scope, int include_attribute)
{
	/* this is similar to parse_btype() initial test logic */
	switch(curtok){
		default:
			return curtok_is_type_qual()
				|| curtok_is_decl_store()
				|| curtok_is_type_primitive();

		case token_attribute:
			return include_attribute;

		case token_signed:
		case token_unsigned:
		case token_inline:
		case token__Noreturn:
		case token_struct:
		case token_union:
		case token_enum:
		case token_typeof:
		case token___auto_type:
		case token___builtin_va_list:
		case token__Alignas:
			return 1;

		case token___extension__:
		{
			int r;

			/* check for __extension__ <type> */
			while(accept(token___extension__));

			r = parse_at_decl(scope, include_attribute);

			/* only place one back on the token stack,
			 * since that's what it's limited to */
			uneat(token___extension__);

			return r;
		}

		case token_identifier:
			return !!parse_at_tdef(scope);
	}
}

static int parse_at_decl_spec(void)
{
	switch(curtok){
		case token_identifier:
		case token_multiply:
		case token_xor:
		case token_open_paren:
			return 1;
		default:
			return 0;
	}
}

static void btype_set_store(
		enum decl_storage *store, int *pstore_set, enum decl_storage st)
{
	if(!store){
		warn_at_print_error(NULL, "storage unwanted (%s)", decl_store_to_str(st));
		fold_had_error = 1;
		return;
	}

	if(*pstore_set){
		warn_at_print_error(NULL, "second store %s", decl_store_to_str(st));
		fold_had_error = 1;
		return;
	}

	*store |= st;
	*pstore_set = 1;
}

static type *parse_btype_end(
		type *btype, enum type_qualifier qual, int is_noreturn,
		attribute ***attr, symtable *scope, where *w)
{
	parse_add_attr(attr, scope); /* int/struct-A __attr__ */

	btype = type_qualify(btype, qual);

	if(is_noreturn)
		dynarray_add(attr, attribute_new(attr_noreturn));

	btype = type_attributed(btype, *attr);
	attribute_array_release(attr);

	return type_at_where(btype, w);
}

enum parse_btype_flags
{
	PARSE_BTYPE_AUTOTYPE = 1 << 0,
	PARSE_BTYPE_DEFAULT_INT = 1 << 1
};

static type *parse_btype(
		enum decl_storage *store, struct decl_align **palign,
		int newdecl_context, symtable *scope,
		enum parse_btype_flags flags)
{
	where autotype_loc;
	/* *store and *palign should be initialised */
	expr *tdef_typeof = NULL;
	attribute **attr = NULL;
	enum type_qualifier qual = qual_none;
	enum type_primitive primitive = type_int;
	int may_default_type = !!(flags & PARSE_BTYPE_DEFAULT_INT);
	int is_signed = 1, is_inline = 0, is_noreturn = 0, is_va_list = 0;
	int store_set = 0, signed_set = 0;
	decl *tdef_decl = NULL;
	enum
	{
		NONE,
		PRIMITIVE_MAYBE_MORE,
		PRIMITIVE_NO_MORE,
		TYPEDEF,
		TYPEOF,
		AUTOTYPE
	} primitive_mode = NONE;

	while(accept(token___extension__));

	for(;;){
		decl *tdef_decl_test;

		if(curtok_is_type_qual()){
			enum type_qualifier q = curtok_to_type_qualifier();

			if(qual & q){
				cc1_warn_at(NULL, duplicate_declspec, "duplicate '%s' specifier", type_qual_to_str(q, 0));
			}

			qual |= q;
			EAT(curtok);

		}else if(curtok_is_decl_store()){

			btype_set_store(store, &store_set, curtok_to_decl_storage());
			EAT(curtok);

		}else if(curtok_is_type_primitive()){
			const enum type_primitive got = curtok_to_type_primitive();

			switch(primitive_mode){
				case PRIMITIVE_MAYBE_MORE:
					/* allow "long int" and "short int" */
#define INT(x)   x == type_int
#define SHORT(x) x == type_short
#define LONG(x)  x == type_long
#define LLONG(x) x == type_llong
#define DBL(x)   x == type_double

					if(      INT(got) && (SHORT(primitive) || LONG(primitive))){
						/* fine, ignore the int */
					}else if(INT(primitive) && (SHORT(got) || LONG(got))){
						primitive = got;
					}else{
						int die = 1;

						if(primitive_mode == PRIMITIVE_MAYBE_MORE){
							/* special case for long long and long double */
							if(LONG(primitive) && LONG(got))
								primitive = type_llong, die = 0; /* allow "int" after this */
							else if(LLONG(primitive) && INT(got))
								primitive_mode = PRIMITIVE_NO_MORE, die = 0;
							else if((LONG(primitive) && DBL(got)) || (DBL(primitive) && LONG(got)))
								primitive = type_ldouble, die = 0, primitive_mode = PRIMITIVE_NO_MORE;
						}

						if(die)
				case PRIMITIVE_NO_MORE:
							die_at(NULL, "second type primitive %s", type_primitive_to_str(got));
					}
					if(primitive_mode == PRIMITIVE_MAYBE_MORE){
						switch(primitive){
							case type_int:
							case type_long:
							case type_llong:
								/* allow short int, long int and long long and long long int */
								break;
							default:
								primitive_mode = PRIMITIVE_NO_MORE;
						}
					}
					break;

				case NONE:
					primitive = got;
					primitive_mode = PRIMITIVE_MAYBE_MORE;
					break;
				case TYPEDEF:
				case TYPEOF:
				case AUTOTYPE:
					die_at(NULL, "type primitive (%s) with %s",
							type_primitive_to_str(primitive),
							primitive_mode == TYPEDEF
							? "typedef-instance"
							: primitive_mode == TYPEOF
							? "typeof"
							: "__auto_type");
			}

			EAT(curtok);

		}else if(curtok == token_signed || curtok == token_unsigned){
			is_signed = curtok == token_signed;

			if(signed_set)
				die_at(NULL, "unwanted second \"%ssigned\"", is_signed ? "" : "un");

			signed_set = 1;
			EAT(curtok);

		}else if(curtok == token_inline){
			if(store)
				*store |= store_inline;
			else
				die_at(NULL, "inline not wanted");

			is_inline = 1;
			EAT(curtok);

		}else if(curtok == token__Noreturn){
			is_noreturn = 1;
			EAT(curtok);

		}else if(curtok == token_struct
				|| curtok == token_union
				|| curtok == token_enum)
		{
			const enum token tok = curtok;
			const char *str;
			type *tref;
			int is_qual;
			where w;

			where_cc1_current(&w);

			EAT(curtok);

			switch(tok){
#define CASE(a)            \
				case token_ ## a:  \
					tref = parse_type_sue(type_ ## a, scope, newdecl_context); \
					str = #a;        \
					break

				CASE(enum);
				CASE(struct);
				CASE(union);

				default:
					ICE("wat");
			}

			if(signed_set || primitive_mode != NONE)
				die_at(NULL, "previous specifier with %s", str);

			/*
			 * struct A { ... } const x;
			 * accept qualifiers and storage for the type, not decl
			 */
			while((is_qual = curtok_is_type_qual()) || curtok_is_decl_store()){
				if(is_qual)
					qual |= curtok_to_type_qualifier();
				else
					btype_set_store(store, &store_set, curtok_to_decl_storage());
				EAT(curtok);
			}

			/* *store is assigned elsewhere */
			/* a _Noreturn function returning a sue is pretty daft... */
			return parse_btype_end(tref, qual, is_noreturn, &attr, scope, &w);

		}else if(curtok == token_typeof){
			if(primitive_mode != NONE)
				die_at(NULL, "typeof specifier after previous specifier");

			cc1_warn_at(NULL, gnu_typeof, "use of GNU typeof()");

			tdef_typeof = parse_expr_sizeof_typeof_alignof(scope);
			primitive_mode = TYPEOF;

		}else if(accept(token___builtin_va_list)){
			if(primitive_mode != NONE)
				die_at(NULL, "can't combine previous specifier with va_list");

			primitive_mode = PRIMITIVE_NO_MORE;
			is_va_list = 1;
			primitive = type_struct;

		}else if(!signed_set /* can't sign a typedef */
		&& (tdef_decl_test = parse_at_tdef(scope)))
		{
			/* typedef name or decl:
			 * if we find a decl named this in our scope,
			 * we're a reference to that, not a type, e.g.
			 * typedef int td;
			 * {
			 *   int td;
			 *   td = 2; // "td" found as typedef + decl
			 * }
			 */

			if(primitive_mode != NONE)
				break; /* already got a primitive
								* e.g. typedef int td
								*      { short td; }
								*/

			assert((tdef_decl_test->store & STORE_MASK_STORE) == store_typedef);

			tdef_decl = tdef_decl_test;
			tdef_typeof = expr_new_sizeof_type(tdef_decl->ref, what_typeof);

			primitive_mode = TYPEDEF;

			EAT(token_identifier);

		}else if(curtok == token_attribute){
			parse_add_attr(&attr, scope); /* __attr__ int ... */
			may_default_type = 1;
			/*
			 * can't depend on !!attr, since it is null when:
			 * __attribute__(());
			 */

		}else if(curtok == token__Alignas){
			struct decl_align *da, **psave;
			type *as_ty;

			if(!palign)
				die_at(NULL, "%s unexpected", token_to_str(curtok));

			for(psave = palign; *psave; psave = &(*psave)->next);

			da = *psave = umalloc(sizeof **palign);

			EAT(token__Alignas);
			EAT(token_open_paren);

			if((as_ty = parse_type(newdecl_context, scope))){
				da->as_int = 0;
				da->bits.align_ty = as_ty;
			}else{
				da->as_int = 1;
				da->bits.align_intk = parse_expr_exp(scope, 0);
			}

			EAT(token_close_paren);

		}else if(accept_where(token___auto_type, &autotype_loc)){
			if(primitive_mode != NONE){
				warn_at_print_error(&autotype_loc,
						"can't combine __auto_type with previous type specifiers");
				parse_had_error = 1;
				continue;
			}
			primitive_mode = AUTOTYPE;

		}else{
			break;
		}
	}

	if(qual != qual_none
	|| store_set
	|| primitive_mode != NONE
	|| signed_set
	|| tdef_typeof
	|| is_inline
	|| may_default_type
	|| is_noreturn
	|| (palign && *palign))
	{
		where w;
		type *r;

		where_cc1_current(&w);

		if(signed_set){
			switch(primitive){
				case type__Bool:
				case type_void:
				case type_float:
				case type_double:
				case type_ldouble:
					die_at(NULL, "%ssigned with %s",
							is_signed ? "" : "un",
							type_primitive_to_str(primitive));
					break;

				case type_struct:
				case type_union:
				case type_enum:
				case type_unknown:
					ucc_unreach(NULL);

				case type_schar:
				case type_uchar:
					ucc_unreach(NULL);
				case type_nchar:
					primitive = is_signed ? type_schar : type_uchar;
					break;

				case type_uint:
				case type_ushort:
				case type_ulong:
				case type_ullong:
					ICE("parsed unsigned type?");
				case type_int:
				case type_short:
				case type_long:
				case type_llong:
					if(!is_signed)
						primitive = TYPE_PRIMITIVE_TO_UNSIGNED(primitive);
					break;
			}
		}

		if(is_va_list){
			r = type_nav_va_list(cc1_type_nav, scope);

		}else switch(primitive_mode){
			case AUTOTYPE:
				UCC_ASSERT(!tdef_typeof, "typedef with __auto_type?");

				r = NULL;
				if(signed_set){
					warn_at_print_error(&autotype_loc,
							"__auto_type given with previous type specifiers");

				}else if(!(flags & PARSE_BTYPE_AUTOTYPE)){
					warn_at_print_error(&autotype_loc, "__auto_type not wanted here");

				}else{
					r = type_nav_auto(cc1_type_nav);
				}

				if(!r){
					/* error case */
					r = type_nav_btype(cc1_type_nav, type_int);
					parse_had_error = 1;
				}

				break;

			case TYPEDEF:
			case TYPEOF:
				UCC_ASSERT(tdef_typeof, "no tdef_typeof for typedef/typeof");
				/* signed size_t x; */
				if(signed_set){
					signed_set = 0;
					warn_at_print_error(NULL,
							"typedef instance can't be signed or unsigned");
				}

				if(tdef_decl) /* typedef only */
					fold_decl(tdef_decl, scope);
				fold_expr_nodecay(tdef_typeof, scope);

				r = type_tdef_of(tdef_typeof, tdef_decl);
				break;

			case PRIMITIVE_NO_MORE:
			case PRIMITIVE_MAYBE_MORE:
			case NONE:
				if(primitive_mode != NONE && primitive == type_llong)
					C99_LONGLONG();

				if(primitive_mode == NONE && !signed_set)
					primitive = type_int;

				r = type_nav_btype(cc1_type_nav, primitive);
				break;
		}

		if(store
		&& STORE_IS_TYPEDEF(*store))
		{
			if(palign && *palign){
				die_at(NULL, "typedefs can't be aligned");
			}
			if(*store & store_inline){
				warn_at_print_error(NULL, "typedef has inline specified");
				fold_had_error = 1;
			}
		}

		return parse_btype_end(r, qual, is_noreturn, &attr, scope, &w);
	}else{
		return NULL;
	}
}

static decl *parse_arg_decl(symtable *scope)
{
	/* argument decls can default to int */
	const enum decl_mode flags = DECL_CAN_DEFAULT | DECL_ALLOW_STORE;
	enum decl_storage store = store_default;
	decl *argdecl;
	type *btype = parse_btype(
			&store, /*align:*/NULL, /*newdecl:*/0, scope,
			PARSE_BTYPE_DEFAULT_INT);

	assert(btype);

	/* don't use parse_decl() - we don't want it folding yet,
	 * things like inits are caught later */
	argdecl = parse_decl_stored_aligned(
			btype, flags,
			store /* register is a valid argument store */,
			/*align:*/NULL,
			scope, NULL, /*is_arg*/1);

	if(!argdecl)
		die_at(NULL, "type expected (got %s)", token_to_str(curtok));

	return argdecl;
}

funcargs *parse_func_arglist(symtable *scope)
{
	funcargs *args = funcargs_new();

	if(curtok == token_close_paren){
		args->args_old_proto = 1;
		cc1_warn_at(NULL, implicit_old_func,
				"old-style function declaration (needs \"(void)\")");
		goto empty_func;
	}

	/* we allow default-to-int here, but need to make
	 * sure we also handle old functions.
	 *
	 * if we have an ident that isn't a typedef, it's an old-func
	 *
	 * f( <here>  (int)) = f(int (int)) = f(int (*)(int))
	 * f( <here> ident) -> old function
	 */
	if(curtok != token_identifier || parse_at_tdef(scope)){
		decl *argdecl = parse_arg_decl(scope);
		type *ty_v = type_is(argdecl->ref, type_btype);

		/* check for x(void) (or an equivalent typedef) */
		/* can't use type_is, since that requires folding */
		if(ty_v
		&& ty_v->bits.type->primitive == type_void
		&& !argdecl->spel)
		{
			/* x(void); */
			funcargs_empty(args);
			args->args_void = 1; /* (void) vs () */

			/* argdecl isn't leaked - it remains in scope, but nameless */
			goto fin;
		}

		for(;;){
			dynarray_add(&args->arglist, argdecl);

			/* add to scope */
			symtab_add_to_scope(scope, argdecl);
			fold_decl(argdecl, scope);

			if(curtok == token_close_paren)
				break;

			EAT_OR_DIE(token_comma);

			if(accept(token_elipsis)){
				args->variadic = 1;
				break;
			}

			/* we allow implicit int, but there must be something,
			 * i.e.
			 * f(int, );
			 * is invalid, as is:
			 * f(int, , int);
			 */
			if(curtok == token_close_paren || curtok == token_comma){
				warn_at_print_error(NULL, "parameter expected");
				fold_had_error = 1;
			}

			/* continue loop */
			argdecl = parse_arg_decl(scope);
		}

fin:;

	}else{
		/* old func - list of idents */
		do{
			decl *d = decl_new();

			if(curtok != token_identifier)
				EAT(token_identifier); /* error */

			d->ref = type_nav_btype(cc1_type_nav, type_int);

			d->spel = token_current_spel();
			dynarray_add(&args->arglist, d);

			symtab_add_to_scope(scope, d);

			EAT(token_identifier);

			if(curtok == token_close_paren)
				break;

			EAT_OR_DIE(token_comma);
		}while(1);

		cc1_warn_at(NULL, omitted_param_types,
				"old-style function declaration");
		args->args_old_proto = 1;
	}

empty_func:
	return args;
}

/*

declarator:
		|  '*' declarator
		|  '*' type_qualifier_list declarator
		|  C_NAME
		|  '(' attr_spec_list declarator ')'
		|  '(' declarator ')'
		|  declarator '[' e ']'
		|  declarator '[' ']'
		|  declarator '(' parameter_type_list ')'
		|  declarator '(' identifier_list ')'
		|  declarator '(' ')'
		;
*/

typedef struct type_parsed type_parsed;
struct type_parsed
{
	where where;

	enum type_parsed_kind
	{
		PARSED_PTR,
		PARSED_FUNC,
		PARSED_ARRAY,
		PARSED_ATTR
	} type;

	attribute **attr;

	union
	{
		struct
		{
			type *(*maker)(type *);
			attribute **attr;
			enum type_qualifier qual;
		} ptr;
		struct
		{
			funcargs *arglist;
			symtable *scope;
		} func;
		struct
		{
			expr *size;
			enum type_qualifier qual;
			unsigned is_static : 1, is_vla : 2;
		} array;
	} bits;

	type_parsed *prev;
};

static type_parsed *parsed_type_ptr(
		enum decl_mode mode, decl *dfor,
		type_parsed *base, symtable *);
#define parsed_type_declarator parsed_type_ptr

static type_parsed *type_parsed_new(
		enum type_parsed_kind kind, type_parsed *prev)
{
	type_parsed *tp = umalloc(sizeof *tp);
	tp->type = kind;
	tp->prev = prev;
	where_cc1_current(&tp->where);
	return tp;
}

static type_parsed *parsed_type_nest(
		enum decl_mode mode, decl *dfor, type_parsed *base, symtable *scope)
{
	if(accept(token_open_paren)){
		type_parsed *ret;
		attribute **attr = NULL;

		/*
		 * we could be here:
		 * int (int a) - from either "^int(int...)" or "void f(int (int));"
		 *                                ^                        ^
		 * in which case, we've read the first "int", stop early, and unget the open paren
		 *
		 * we don't look for open parens - they're used for nexting, e.g.
		 * int ((*p)(void));
		 *
		 * int (__attribute ...)
		 *      ^ we either parse this as an anon-function type or an
		 *      attributed function:
		 *      int (__attribute(())); // int (int)
		 *      int (__attribute(()) f)() // int f()
		 *
		 * if we're parsing a decl, we reject __attribute as an argument spec
		 */
		if(parse_at_decl(scope, !dfor) || curtok == token_close_paren){
			/* int() or char(short) - func decl */
			uneat(token_open_paren);
			/* parse_...func will grab this as funcargs instead */
			return base;
		}

		/* int (__attribute(()) f)() ... */
		parse_add_attr(&attr, scope);

		ret = parsed_type_declarator(mode, dfor, base, scope);

		EAT(token_close_paren);

		if(attr){
			ret = type_parsed_new(PARSED_ATTR, ret);
			ret->attr = attr;
			attr = NULL;
		}


		return ret;

	}else if(curtok == token_identifier){
		if(!dfor)
			die_at(NULL, "identifier unexpected");

		/* set spel + location info */
		where_cc1_current(&dfor->where);
		dfor->spel = token_current_spel();
		where_cc1_adj_identifier(&dfor->where, dfor->spel);

		EAT(token_identifier);

	}else if(mode & DECL_SPEL_NEED){
		die_at(NULL, "need identifier for decl");
	}

	return base;
}

static type_parsed *parsed_type_array(
		enum decl_mode mode, decl *dfor, type_parsed *base, symtable *scope)
{
	type_parsed *r = parsed_type_nest(mode, dfor, base, scope);

	while(accept(token_open_square)){
		expr *size = NULL;
		enum type_qualifier q = qual_none;
		int is_static = 0, is_vla = 0;

		/* parse int x[restrict|static ...] */
		for(;;){
			if(curtok_is_type_qual())
				q |= curtok_to_type_qualifier();
			else if(curtok == token_static)
				is_static++;
			else
				break;

			EAT(curtok);
		}

		if(accept(token_close_square)){
			/* null size */
		}else{
			/* fold.c checks for const-ness */
			/* grammar says it's a conditional here, hence no-comma */
			int is_star = 0;

			/* look for [*] */
			if(accept(token_multiply)){
				if(accept(token_close_square)){
					is_star = 1;
				}else{
					uneat(token_multiply);
				}
			}

			if(!is_star){
				size = PARSE_EXPR_CONSTANT(scope, 0);
				EAT(token_close_square);

				FOLD_EXPR(size, scope);

				if(!type_is_integral(size->tree_type)){
					parse_had_error = 1;
					warn_at_print_error(&size->where,
							"array size isn't integral (%s)",
							type_to_str(size->tree_type));
					size = NULL;
				}else{
					consty k;
					const_fold(size, &k);

					/* if it's not a constant number, or it is, but it's non-standard
					 * (and we don't have the fold-const-vlas setting), then treat
					 * as a vla */
					if(k.type != CONST_NUM
					|| (k.nonstandard_const && !(cc1_fopt.fold_const_vlas)))
					{
						is_vla = VLA;
					}
					else if(!K_INTEGRAL(k.bits.num))
					{
						die_at(NULL, "not an integral array size");
					}
				}
			}else{
				is_vla = VLA_STAR;
			}
		}

		if(is_static > 1)
			die_at(NULL, "multiple static specifiers in array size");

		r = type_parsed_new(PARSED_ARRAY, r);
		r->bits.array.size = size;
		r->bits.array.qual = q;
		r->bits.array.is_static = is_static;
		r->bits.array.is_vla = is_vla;
	}

	return r;
}

static type_parsed *parsed_type_func(
		enum decl_mode mode, decl *dfor, type_parsed *base, symtable *scope)
{
	type_parsed *sub = parsed_type_array(mode, dfor, base, scope);

	while(accept(token_open_paren)){
		symtable *subscope = symtab_new(scope, where_cc1_current(NULL));

		subscope->are_params = 1;

		sub = type_parsed_new(PARSED_FUNC, sub);
		sub->bits.func.arglist = parse_func_arglist(subscope);
		sub->bits.func.scope = subscope;

		EAT(token_close_paren);
	}

	return sub;
}

static type_parsed *parsed_type_ptr(
		enum decl_mode mode, decl *dfor, type_parsed *base, symtable *scope)
{
	int ptr;

	if((ptr = accept(token_multiply)) || accept(token_xor)){
		type *(*maker)(type *) = ptr ? type_ptr_to : type_block_of;

		type_parsed *r_ptr;
		attribute **attr = NULL;
		type_parsed *sub;

		enum type_qualifier qual = qual_none;

		while(curtok_is_type_qual() || curtok == token_attribute){
			if(curtok == token_attribute){
				parse_add_attr(&attr, scope);
			}else{
				qual |= curtok_to_type_qualifier();
				EAT(curtok);
			}
		}

		r_ptr = type_parsed_new(PARSED_PTR, NULL);
		r_ptr->bits.ptr.maker = maker;
		r_ptr->bits.ptr.attr = attr; /* pass ownership */ attr = NULL;
		r_ptr->bits.ptr.qual = qual;

		/* this is essentially
		 * r_ptr = maker(parse_type(...), qual);
		 * i.e. we wait until we've parsed the sub-bit then insert ourselves
		 */
		sub = parsed_type_declarator(mode, dfor, NULL, scope);
		if(sub){
			type_parsed *i;

			r_ptr->prev = sub;
			for(i = sub; i->prev; i = i->prev);
			i->prev = base;

			return r_ptr;
		}else{
			r_ptr->prev = base;
			return r_ptr;
		}
	}else{
		return parsed_type_func(mode, dfor, base, scope);
	}
}

static type *parse_type_declarator_to_type(
		enum decl_mode mode, decl *dfor, type *base, symtable *scope)
{
	type_parsed *parsed = parsed_type_declarator(mode, dfor, NULL, scope);
	type_parsed *i, *tofree;
	type *ty = base;

	for(i = parsed; i; tofree = i, i = i->prev, free(tofree)){
		enum type_qualifier qual = qual_none;

		switch(i->type){
			case PARSED_PTR:
				ty = i->bits.ptr.maker(ty);
				qual = i->bits.ptr.qual;
				break;
			case PARSED_ARRAY:
				qual = i->bits.array.qual;

				if(i->bits.array.is_vla){
					if(i->bits.array.is_static){
						warn_at_print_error(NULL,
								"'static' can't be used with a star-modified array");
						fold_had_error = 1;
					}

					ty = type_vla_of(
							ty, i->bits.array.size,
							i->bits.array.is_vla);

				}else{
					ty = type_array_of_static(
							ty,
							i->bits.array.size,
							i->bits.array.is_static);
				}
				assert(ty->type == type_array);
				assert(ty->bits.array.is_vla == i->bits.array.is_vla);

				if(i->bits.array.is_static && i->prev){
					fold_had_error = 1;
					warn_at_print_error(&i->where,
							"static in non outermost array type");
				}
				break;
			case PARSED_FUNC:
				ty = type_func_of(
						ty,
						i->bits.func.arglist,
						i->bits.func.scope);
				break;

			case PARSED_ATTR:
				/* attribute applied below */
				break;
		}

		/* don't transform arrays from int[const] -> int const[],
		 * C11 6.7.3.9 specifies this, but we don't do it yet as
		 * we want int[const] for function parameters, so we can
		 * convert them to int *const.
		 *
		 * additionally, int[const] is invalid anywhere else, so
		 * we don't transform so this can also be detected.
		 *
		 * const qualification of arrays can therefore only happen
		 * through typedefs:
		 * typedef int array[3];
		 * const array x;
		 *
		 * here type_qualify() is used to create x's type, not
		 * type_qualify_transform_array(), meaning the transformation
		 * is applied correctly where needed
		 */
		ty = type_attributed(
				type_qualify_transform_array(
					type_at_where(ty, &i->where),
					qual, 0),
				i->attr);

		attribute_array_release(&i->attr);
	}

	if(dfor)
		ty = type_at_where(ty, &dfor->where);
	return ty;
}

static type *parse_type_declarator(
		enum decl_mode mode, decl *dfor, type *base, symtable *scope,
		int *try_trail)
{
	type *t = parse_type_declarator_to_type(mode, dfor, base, scope);
	type *ttrail;
	type *fnty;
	where ptr_loc;

	if(!*try_trail || !accept_where(token_ptr, &ptr_loc)){
		*try_trail = 0;
		return t;
	}

	/* try to parse trail using topmost function's scope */
	fnty = type_is(t, type_func);
	if(fnty)
		scope = fnty->bits.func.arg_scope;

	ttrail = parse_type(/*newdecl:*/1, scope);

	if(!ttrail){
		warn_at_print_error(&ptr_loc, "trailing return type expected");
		parse_had_error = 1;
		return t;
	}

	return type_nav_changeauto(t, ttrail);
}

type *parse_type(int newdecl, symtable *scope)
{
	type *btype = NULL;
	int try_trail = 0;

	if(accept(token_auto)){
		/* auto <non-ident> is fine, but
		 * auto int, or auto myident
		 * needs to be interpreted as in C */
		if(parse_at_decl(scope, 1)){
			uneat(token_auto);
		}else{
			btype = type_nav_btype(cc1_type_nav, type_int);
			try_trail = 1;
		}
	}

	if(!btype)
		btype = parse_btype(NULL, NULL, newdecl, scope, 0);

	if(!btype)
		return NULL;

	return parse_type_declarator(0, NULL, btype, scope, &try_trail);
}

type **parse_type_list(symtable *scope)
{
	type **types = NULL;

	if(curtok == token_close_paren)
		return types;

	do{
		type *r = parse_type(0, scope);

		if(!r)
			die_at(NULL, "type expected");

		dynarray_add(&types, r);
	}while(accept(token_comma));

	return types;
}

static void parse_add_asm(decl *d)
{
	if(accept(token_asm)){
		decl *const proto = decl_proto(d);
		char *rename, *p;
		struct cstring *cstr;

		EAT(token_open_paren);

		if(curtok != token_string)
			die_at(NULL, "string expected");

		cstr = parse_asciz_str();
		if(!cstr)
			return;

		rename = cstring_detach(cstr);
		EAT(token_string);

		EAT(token_close_paren);

		/* only allow [0-9A-Za-z_.] */
		for(p = rename; *p; p++)
			if(!isalnum(*p) && *p != '_' && *p != '.' && *p != '$'){
				cc1_warn_at(NULL, asm_badchar, "asm name contains character 0x%x", *p);
				break;
			}

		if(d->spel_asm){
			if(strcmp(d->spel_asm, rename)){
				warn_at_print_error(&d->where,
						"decl \"%s\" already has an asm() name (\"%s\")",
						d->spel, d->spel_asm);
				if(proto && proto != d)
					note_at(&proto->where, "previous declaration here");
				parse_had_error = 1;
			}
			free(rename);
			rename = NULL;
		}else{
			d->spel_asm = rename;
		}
	}
}

static void parsed_decl(decl *d, symtable *scope, int is_arg)
{
	where *loc = type_has_loc(d->ref);
	if(!loc)
		loc = &d->where;

	fold_type_ondecl_w(d, scope, loc, is_arg);
}

static decl *parse_decl_stored_aligned(
		type *btype, enum decl_mode mode,
		enum decl_storage store, struct decl_align *align,
		symtable *scope, symtable *add_to_scope,
		int is_arg)
{
	decl *d = decl_new();
	where w_eq;
	int is_autotype = type_is_autotype(btype);

	d->store = store; /* set early for parse_type_declarator() */

	/* int __attr__ spel, __attr__ spel2, ... */
	parse_add_attr(&d->attr, scope);

	if(is_autotype){
		d->spel = token_current_spel();
		EAT(token_identifier);

	}else{
		/* allow extra specifiers */
		int try_trail = 0;

		if((store & STORE_MASK_STORE) == store_auto){
			const struct btype *bt = type_get_type(btype);
			/* auto, defaulted to int? */
			if(bt && bt->primitive == type_int){
				/* if there are no more specs/quals... */
				try_trail = !parse_at_decl(scope, 1);
			}
		}

		d->ref = parse_type_declarator(mode, d, btype, scope, &try_trail);

		if(try_trail){
			/* got trailing type - remove auto store */
			d->store = store_default | (d->store & STORE_MASK_EXTRA);
		}

		if(add_to_scope)
			symtab_add_to_scope(add_to_scope, d);
	}

	/* only check if it's not a function, otherwise it could be
	 * int f(i)
	 *   __attribute__(()) int i;
	 * {
	 * }
	 */

	if(is_autotype || !type_is(d->ref, type_func)){
		/* parse __asm__ naming before attributes, as per gcc and clang */
		parse_add_asm(d);
		parse_add_attr(&d->attr, scope); /* int spel __attr__ */

		/* now we have attributes, etc... */
		parsed_decl(d, scope, is_arg);

		if(d->spel && accept_where(token_assign, &w_eq)){
			int static_ctx = !scope->parent ||
				(store & STORE_MASK_STORE) == store_static;

			if(!is_autotype && add_to_scope){
				/* need to add its symbol early,
				 * in case it's mentioned in the init */
				fold_decl_add_sym(d, add_to_scope);
			}

			d->bits.var.init.dinit = parse_init(scope, static_ctx);

			/* top-level inits have their .where on the '=' token */
			memcpy_safe(&d->bits.var.init.dinit->where, &w_eq);

			if(is_autotype){
				decl_init *init = d->bits.var.init.dinit;

				/* delayed add-to-scope */
				if(add_to_scope)
					symtab_add_to_scope(add_to_scope, d);

				UCC_ASSERT(!d->ref, "already have decl type?");

				if(init->type != decl_init_scalar){
					warn_at_print_error(&d->where, "bad initialiser for __auto_type");
				}else{
					attribute **attr = NULL;
					type *attr_node;

					/* gcc decays "" to char* */
					FOLD_EXPR(init->bits.expr, scope);

					attr_node = type_skip_non_attr(btype);
					if(attr_node && attr_node->type == type_attr)
						attr = attribute_array_retain(attr_node->bits.attr);

					/* need to preserve __attribute__ and qualifiers
					 * storage and alignment are kept on the decl */
					d->ref = type_attributed(
							type_qualify(
								init->bits.expr->tree_type,
								type_qual(type_at_where(btype, &init->where))),
							attr);

					attribute_array_release(&attr);

					parsed_decl(d, scope, is_arg);
				}
			}

		}else if(is_autotype){
			warn_at_print_error(&d->where, "__auto_type without initialiser");
			UCC_ASSERT(!d->ref, "already have decl type?");
		}

		if(!d->ref){
			parse_had_error = 1;
			d->ref = type_nav_btype(cc1_type_nav, type_int);
		}
	}

	if(!type_is(d->ref, type_func)){
		d->bits.var.align = align;
	}else if(align){
		warn_at_print_error(&d->where,
				"alignment specified for function '%s'",
				d->spel);

		fold_had_error = 1;
	}

	/* copy all of d's attributes to the .ref, so that function
	 * types get everything correctly. */
	d->ref = type_attributed(d->ref, d->attr);

	return d;
}

static void prevent_typedef(enum decl_storage store)
{
	if(store_typedef == (store & STORE_MASK_STORE))
		die_at(NULL, "typedef unexpected");
}

static type *default_type(void)
{
	cc1_warn_at(NULL, implicit_int, "defaulting type to int");

	return type_nav_btype(cc1_type_nav, type_int);
}

static void unused_attributes(decl *dfor, attribute **attr)
{
	char buf[64];
	struct_union_enum_st *sue;

	if(!attr)
		return;

	assert(!dfor || !dfor->spel);

	if(dfor && (sue = type_is_s_or_u_or_e(dfor->ref))){
		snprintf(buf, sizeof buf,
				" (place attribute after '%s')",
				sue_str(sue));
	}


	cc1_warn_at(&attr[0]->where, ignored_attribute,
			"attribute ignored - no declaration%s", buf);
}

decl *parse_decl(
		enum decl_mode mode, int newdecl,
		symtable *scope, symtable *add_to_scope)
{
	attribute **decl_attr = NULL;
	enum decl_storage store = store_default;
	type *bt;
	decl *d;
	enum parse_btype_flags flags = PARSE_BTYPE_AUTOTYPE;
	int got_attr;

	parse_add_attr_out(&decl_attr, scope, &got_attr);
	if(got_attr)
		flags |= PARSE_BTYPE_DEFAULT_INT;

	bt = parse_btype(
			mode & DECL_ALLOW_STORE ? &store : NULL,
			/*align:*/NULL,
			newdecl, scope, flags);

	if(!bt){
		if((mode & DECL_CAN_DEFAULT) == 0){
			unused_attributes(NULL, decl_attr);
			attribute_array_release(&decl_attr);
			return NULL;
		}

		bt = default_type();

	}else{
		prevent_typedef(store);
	}

	d = parse_decl_stored_aligned(
			type_attributed(bt, decl_attr),
			mode,
			store, NULL /* align */,
			scope, add_to_scope, 0);

	attribute_array_release(&decl_attr);

	fold_decl(d, scope);

	return d;
}

static int is_old_func(decl *d)
{
	type *r = type_is(d->ref, type_func);
	/* don't treat int f(); as an old function */
	return r && funcargs_is_old_func(r->bits.func.args);
}

static void check_and_replace_old_func(decl *d, decl **old_args, symtable *scope)
{
	/* check then replace old args */
	int n_proto_decls, n_old_args;
	int i;
	funcargs *dfuncargs = type_is_func_or_block(d->ref)->bits.func.args;

	UCC_ASSERT(type_is(d->ref, type_func), "not func");

	if(!dfuncargs->args_old_proto){
		die_at(&d->where, dfuncargs->arglist
				? "unexpected old-style decls - new style proto used"
				: "parameters specified despite empty declaration in prototype");
	}

	n_proto_decls = dynarray_count(dfuncargs->arglist);
	n_old_args = dynarray_count(old_args);

	if(n_old_args > n_proto_decls)
		die_at(&d->where, "old-style function decl: too many decls");

	for(i = 0; i < n_old_args; i++){
		int j, found = 0;

		if(!type_is(old_args[i]->ref, type_func)
		&& old_args[i]->bits.var.init.dinit)
		{
			die_at(&old_args[i]->where,
					"parameter \"%s\" is initialised",
					old_args[i]->spel);
		}

		for(j = 0; j < n_proto_decls; j++){
			if(!strcmp(old_args[i]->spel, dfuncargs->arglist[j]->spel)){

				/* replace the old implicit int arg's type
				 *
				 * note we don't replace the decl itself, since that's
				 * now in an argument symtable somewhere in scope too
				 */
				decl_replace_with(dfuncargs->arglist[j], old_args[i]);

				/* TODO: remove old_args[i] from scope
				 * (needs refactor to remove symtable from types) */
				/*decl_free(old_args[i]);*/
				/*old_args[i] = NULL;*/

				found = 1;
				break;
			}
		}

		if(!found)
			die_at(&old_args[i]->where, "no such parameter '%s'", old_args[i]->spel);
	}

	/* need to re-decay funcargs, etc.
	 * f(i)
	 *   int i[];
	 * { ... }
	 * will decay the implicit "int i", but now it's been replaced with "int i[]"
	 */
	fold_funcargs(dfuncargs, scope, NULL);
}

static void check_function_storage_redef(decl *new, decl *old)
{
	/* C11 6.2.2 */
	decl *d;

	/* if the first is static, then we ignore storage */
	for(d = old; d->proto; d = d->proto);

	if((d->store & STORE_MASK_STORE) == store_static)
		return;

	/* can't redefine as static now */
	if((new->store & STORE_MASK_STORE) == store_static){
		char buf[WHERE_BUF_SIZ];

		warn_at_print_error(&new->where,
				"static redefinition of non-static \"%s\"\n"
				"%s: note: previous definition",
				new->spel, where_str_r(buf, &old->where));
		fold_had_error = 1;
	}
}

static void check_var_storage_redef(decl *new, decl *old)
{
	/* C99 6.2.2
	 * 5) [...] If the declaration of an identifier for an object has file scope
	 * and no storage-class specifier, its linkage is external.
	 *
	 * first decl: if static, can't have default storage (unless function)
	 *             if extern, can have anything but static
	 */
	int first_is_static;
	int is_fn;
	int error = 0;

	old = decl_proto(old);
	is_fn = !!type_is(old->ref, type_func);

	first_is_static = (store_static == (enum decl_storage)(old->store & STORE_MASK_STORE));

	switch((enum decl_storage)(new->store & STORE_MASK_STORE)){
		default:
			return;
		case store_default:
			if(first_is_static && !is_fn)
				error = 1;
			break;
		case store_static:
			if(!first_is_static)
				error = 1;
			break;
		case store_extern:
			/* fine */
			break;
	}

	if(error){
		char buf[WHERE_BUF_SIZ];
		warn_at_print_error(&new->where,
				"static redefinition as non-static\n"
				"%s: note: previous definition",
				new->spel,
				where_str_r(buf, &old->where));
		fold_had_error = 1;
	}
}

static void decl_pull_to_func(decl *const d_this, decl *const d_prev)
{
	char wbuf[WHERE_BUF_SIZ];

	if(!type_is(d_prev->ref, type_func))
		return; /* error caught later */

	check_function_storage_redef(d_this, d_prev);

	if(d_prev->bits.func.code){
		/* just warn that we have a declaration
		 * after a definition, then get out.
		 *
		 * any declarations after this aren't warned about
		 * (since d_prev is different), but one warning is fine
		 *
		 * special case: we allow changing a pure inline function
		 * to extern- or static-inline (provided it doesn't change
		 * anything else like asm() renames)
		 */
		if((d_prev->store & store_inline) && !d_this->spel_asm){
			/* redefinition is fine.
			 * type errors are caught later on in the decl folding stage
			 */
		}else{
			cc1_warn_at(&d_this->where,
					ignored_late_decl,
					"declaration of \"%s\" after definition is ignored\n"
					"%s: note: definition here",
					d_this->spel,
					where_str_r(wbuf, &d_prev->where));
			return;
		}
	}

	assert(!d_this->spel_asm && "shouldn't have parsed asm at this point");
	if(d_prev->spel_asm)
		d_this->spel_asm = d_prev->spel_asm;

	/* update the f(void) bools */
	{
		funcargs *fargs_this = type_funcargs(d_this->ref),
		         *fargs_prev = type_funcargs(d_prev->ref);

		fargs_this->args_void          |= fargs_prev->args_void;
		fargs_this->args_void_implicit |= fargs_prev->args_void_implicit;
	}
}

static void warn_for_unaccessible_sue(
		decl *d, enum decl_multi_mode mode)
{
	enum type_qualifier qual;
	struct_union_enum_st *sue;

	/*
	 * struct { int i; }; - continue to next one
	 * this is either struct or union, not enum
	 */
	if(mode & DECL_MULTI_NAMELESS)
		return;

	if(d->spel)
		return;

	/* enums are fine */
	sue = type_is_s_or_u(d->ref);

	if(!sue)
		return;

	if(sue->anon)
		cc1_warn_at(NULL,
				struct_noinstance_anon,
				"anonymous %s with no instances", sue_str(sue));

	/* check for storage/qual on no-instance */
	qual = type_qual(d->ref);
	if(qual || d->store != store_default){
		cc1_warn_at(NULL,
				struct_noinstance_qualified,
				"ignoring %s%s%son no-instance %s",
				d->store != store_default ? decl_store_to_str(d->store) : "",
				d->store != store_default ? " " : "",
				type_qual_to_str(qual, 1),
				sue_str(sue));
	}
}

static int warn_for_unused_typename(
		decl *d, enum decl_multi_mode mode)
{
	struct_union_enum_st *sue = type_is_s_or_u_or_e(d->ref);

	if(sue) switch(sue->primitive){
		case type_struct:
		case type_union:
			/* don't warn for tagged struct/unions */
			if(mode & DECL_MULTI_NAMELESS)
				return 0;

			if(sue->anon)
				break;
			/* don't warn for named structs/unions */
			return 0;

		case type_enum:
			/* don't warn for enums - they're always declarations */
			return 0;

		default:
			break;
	}else if(type_is(d->ref, type_func)){
		warn_at_print_error(&d->where, "unnamed function declaration");
		fold_had_error = 1;
		return 1;
	}

	/* C 6.7/2:
	 * A decl shall declare at least a declarator a tag,
	 * or the members of an enumeration.
	 *
	 * allow nameless as an extension
	 */
	cc1_warn_at(&d->where, decl_nodecl, "declaration doesn't declare anything");

	return 1;
}

static int check_star_modifier_1(type *t, where *w)
{
	assert(t->type == type_array);
	if(t->bits.array.is_vla == VLA_STAR){
		warn_at_print_error(w,
				"star modifier can only appear on prototypes");
		fold_had_error = 1;
		return 1;
	}
	return 0;
}

static void check_star_modifier(symtable *arg_symtab)
{
	decl **i;

	for(i = symtab_decls(arg_symtab); i && *i; i++){
		decl *d = *i;
		type *t = type_is_decayed_array(d->ref);

		if(t && check_star_modifier_1(t, &d->where))
			continue;

		for(t = d->ref; t; t = type_next(t)){
			if(t->type == type_array){
				if(check_star_modifier_1(t, &d->where))
					break;
			}
		}
	}
}

static void parse_post_func(decl *d, symtable *in_scope, int had_post_attr)
{
	int need_func = 0;
	type *func_r;
	symtable *arg_symtab;

	/* special case - support asm directly after a function
	 * no parse ambiguity - asm can only appear at the end of a decl,
	 * before __attribute__
	 */
	if(curtok == token_asm){
		attribute **attrs = NULL;

		if(had_post_attr){
			cc1_warn_at(NULL, gnu_gcc_compat,
					"asm() after __attribute__ (GCC compat)");
		}

		parse_add_asm(d);

		parse_add_attr(&attrs, in_scope);

		if(attrs)
			d->ref = type_attributed(d->ref, attrs);

		attribute_array_release(&attrs);
	}

	if(is_old_func(d)){
		decl **old_args = NULL;
		/* NULL - we don't want these in a scope */
		while(parse_decl_group(
				DECL_MULTI_IS_OLD_ARGS, /*newdecl_context:*/0,
				in_scope,
				NULL, &old_args))
		{
		}

		if(old_args){
			check_and_replace_old_func(d, old_args, in_scope);

			dynarray_free(decl **, old_args, NULL);

			/* old function with decls after the close paren,
			 * need a function */
			need_func = 1;
		}
	}

	func_r = type_is(d->ref, type_func);
	arg_symtab = func_r->bits.func.arg_scope;

	/* clang-style allows __attribute__ and then a function block */
	if(need_func || curtok == token_open_block){
		/* need to set scope to include function argumen
		 * e.g. f(struct A { ... })
		 */
		UCC_ASSERT(func_r, "function expected");

		arg_symtab->in_func = d;

		fold_decl(d, arg_symtab->parent);

		check_star_modifier(arg_symtab);

		d->bits.func.code = parse_stmt_block(arg_symtab, NULL);

		/* if:
		 * f(){...}, then we don't have args_void, but implicitly we do
		 */
		type_funcargs(d->ref)->args_void_implicit = 1;

		if(STORE_IS_TYPEDEF(d->store)){
			warn_at_print_error(&d->where, "typedef storage on function");
			fold_had_error = 1;
		}
	}
}

static void check_missing_proto_extern(decl *d)
{
	/* 'd' has no previous decl */

	if(DECL_IS_HOSTED_MAIN(d))
		return;

	switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
		case store_static:
		case store_typedef:
			return;
		default:
			break;
	}

	if(type_is(d->ref, type_func)){
		if(!d->bits.func.code)
			return; /* only warn if it has code */

		cc1_warn_at(&d->where, missing_prototype,
				"no previous prototype for non-static function");

	}else{
		/* extern is ignored for variables */
		if((d->store & STORE_MASK_STORE) == store_extern)
			return;

		cc1_warn_at(&d->where, missing_variable_decls,
				"non-static declaration has no previous extern declaration");
	}
}

static void link_to_previous_decl(
		decl *d, symtable *in_scope, int *const found_prev_proto)
{
	/* Look for a previous declaration of d->spel.
	 * if found, we pull its asm() and attributes to the current,
	 * thus propagating them down in O(1) to the eventual definition.
	 * Do this before adding, so we don't find 'd'
	 *
	 * This also means any use of d will have the most up to date
	 * attribute information about it
	 */
	struct symtab_entry ent;

	if(symtab_search(in_scope, d->spel, d, &ent) && ent.type == SYMTAB_ENT_DECL){
		/* link the proto chain for __attribute__ checking,
		 * nested function prototype checking and
		 * '.extern fn' code gen easing
		 */
		symtable *const prev_in = ent.owning_symtab;
		decl *const d_prev = ent.bits.decl;
		const int are_functions =
			(!!type_is(d->ref, type_func) +
			 !!type_is(d_prev->ref, type_func));

		*found_prev_proto = 1;

		d->proto = d_prev;
		d_prev->impl = d; /* for inlining */

		switch(are_functions){
			case 2:
				decl_pull_to_func(d, d_prev);

				if(!d_prev->proto){
					funcargs *fa = type_funcargs(d_prev->ref);
					int is_prototype = !FUNCARGS_EMPTY_NOVOID(fa);

					if(!is_prototype)
						*found_prev_proto = 0;
				}
				break;

			case 0:
				/* variable - may or may not be related */
				if(prev_in != in_scope){
					/* unrelated */
					d->proto = NULL;
				}else{
					check_var_storage_redef(d, d_prev);
				}
				break;
			case 1:
				/* unrelated */
				d->proto = NULL;
				break;
		}
	}else{
		*found_prev_proto = 0;
	}

}

static void error_on_unwanted_func(
		decl *d, enum decl_multi_mode mode)
{
	if(!type_is(d->ref, type_func))
		return;

	if(d->bits.func.code && (mode & DECL_MULTI_ACCEPT_FUNC_CODE) == 0)
		die_at(&d->where, "function code not wanted (%s)", d->spel);

	if((mode & DECL_MULTI_ACCEPT_FUNC_DECL) == 0)
		die_at(&d->where, "function declaration not wanted (%s)", d->spel);
}

static int parse_decl_attr(decl *d, symtable *scope)
{
	/* special case - support GCC __attribute__
	 * directly after a "new"/prototype function
	 *
	 * only accept if it's a new-style function,
	 * i.e.
	 *
	 * f(i) __attribute__(()) int i; { ... }
	 *
	 * is invalid, since old-style functions have
	 * decls after the final close-paren
	 */
	if(curtok == token_attribute && !is_old_func(d)){
		/* add to .ref, since this is what is checked
		 * when the function decays to a pointer */
		attribute **attrs = NULL;
		parse_add_attr(&attrs, scope);
		d->ref = type_attributed(d->ref, attrs);
		attribute_array_release(&attrs);
		return 1;
	}
	return 0;
}

static void parse_decl_check_complete(
		decl *d,
		symtable *scope,
		const int is_member)
{
	/* Ensure locally declared types are complete.
	 * Must be done here (instead of stmt_code)
	 * as by the time we reach stmt_code's fold function,
	 * we may have parsed a later-defined struct type,
	 * completing the definition.
	 *
	 * Only check for local variables - i.e. not args, members and globals
	 */

	if(is_member)
		return;

	if(!scope->parent)
		return; /* global */

	if(scope->are_params)
		return; /* arg */

	fold_check_decl_complete(d);
}

int parse_decl_group(
		const enum decl_multi_mode mode,
		int newdecl,
		symtable *in_scope,
		symtable *add_to_scope, decl ***pdecls)
{
	const enum decl_mode parse_flag =
		(mode & DECL_MULTI_CAN_DEFAULT ? DECL_CAN_DEFAULT : 0);

	enum decl_storage store = store_default;
	struct decl_align *align = NULL;
	type *this_ref;
	decl *last = NULL;
	int at_plain_ident;
	attribute **decl_attr = NULL;
	enum parse_btype_flags flags = PARSE_BTYPE_AUTOTYPE;
	int got_attr;

	UCC_ASSERT(add_to_scope || pdecls, "what shall I do?");

	parse_add_attr_out(&decl_attr, in_scope, &got_attr);
	if(got_attr)
		flags |= PARSE_BTYPE_DEFAULT_INT;

	parse_static_assert(in_scope);

	this_ref = parse_btype(
			mode & DECL_MULTI_ALLOW_STORE ? &store : NULL,
			mode & DECL_MULTI_ALLOW_ALIGNAS ? &align : NULL,
			newdecl, in_scope, flags);

	if(!this_ref){
		if(!parse_at_decl_spec() || !(mode & DECL_MULTI_CAN_DEFAULT)){
			unused_attributes(NULL, decl_attr);
			attribute_array_release(&decl_attr);
			return 0;
		}

		this_ref = default_type();
	}

	do{
		int found_prev_proto = 1;
		int had_field_width = 0;
		int done = 0;
		int attr_post_decl;
		decl *d;
		at_plain_ident = (curtok == token_identifier);

		d = parse_decl_stored_aligned(
				this_ref, parse_flag,
				store, align,
				in_scope, add_to_scope, 0);

		if((mode & DECL_MULTI_ACCEPT_FIELD_WIDTH)
		&& accept(token_colon))
		{
			/* normal decl, check field spec */
			d->bits.var.field_width = PARSE_EXPR_CONSTANT(in_scope, 0);
			had_field_width = 1;
		}

		/* need to parse __attribute__ before folding the type */
		attr_post_decl = parse_decl_attr(d, in_scope);

		if(d->spel){
			d->ref = type_attributed(d->ref, decl_attr);
		}else{
			type *attributed;

			unused_attributes(d, decl_attr);

			/* check for unused attribute on the type */
			attributed = type_skip_non_attr(this_ref);
			if(attributed && attributed->type == type_attr)
				unused_attributes(d, attributed->bits.attr);
		}

		attribute_array_release(&decl_attr);

		fold_type_ondecl_w(d, in_scope, NULL, 0);

		if(!d->spel && !had_field_width){
			/*
			 * int; - fine for "int;", but "int i,;" needs to fail
			 * struct A; - fine
			 * struct { int i; }; - warn
			 */
			if(last){
				die_at(&d->where,
						"identifier expected for declaration (got %s)",
						token_to_str(curtok));
			}

			if(warn_for_unused_typename(d, mode)){
				/* ignore the decl */
			}
			done = 1;
		}

		if(d->spel && (mode & DECL_MULTI_IS_STRUCT_UN_MEMB) == 0)
			link_to_previous_decl(d, in_scope, &found_prev_proto);
		if(pdecls)
			dynarray_add(pdecls, d);

		/* now we have the function in scope we parse its code */
		if(type_is(d->ref, type_func))
			parse_post_func(d, in_scope, attr_post_decl);

		if(!in_scope->parent && !found_prev_proto && !(mode & DECL_MULTI_IS_OLD_ARGS))
			check_missing_proto_extern(d);

		error_on_unwanted_func(d, mode);

		warn_for_unaccessible_sue(d, mode);

		/* must fold _after_ we get the bitfield, etc */
		fold_decl_maybe_member(d, in_scope, mode & DECL_MULTI_IS_STRUCT_UN_MEMB);

		parse_decl_check_complete(d, in_scope, mode & DECL_MULTI_IS_STRUCT_UN_MEMB);

		last = d;
		if(done)
			break;
	}while(accept(token_comma));


	if(last && (!type_is(last->ref, type_func) || !last->bits.func.code)){
		/* end of type, if we have an identifier,
		 * '(' or '*', it's an unknown type name */
		if(at_plain_ident && parse_at_decl_spec())
			die_at(&last->where, "unknown type name '%s'", last->spel);
		/* else die here: */
		EAT(token_semicolon);
	}

	attribute_array_release(&decl_attr);

	return 1;
}
