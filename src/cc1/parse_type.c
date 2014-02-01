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

#include "sue.h"
#include "sym.h"

#include "cc1.h"
#include "cc1_where.h"

#include "parse.h"
#include "parse_type.h"

#include "expr.h"
#include "type_nav.h"
#include "type_is.h"

/*#define PARSE_DECL_VERBOSE*/

/* we don't do the type_is_* since it needs to be folded for that */
#define PARSE_type_is(r, ty) ((type_skip_attrs_casts(r)->type == ty) ? (r) : NULL)
#define PARSE_DECL_IS_FUNC(d) PARSE_type_is(type_skip_attrs_casts(d->ref), type_func)

#define PARSE_type_is_s_or_u_or_e(r) PARSE_type_is_s_or_u_or_e2(r, 1)
#define PARSE_type_is_s_or_u(r)      PARSE_type_is_s_or_u_or_e2(r, 0)

static struct_union_enum_st *PARSE_type_is_s_or_u_or_e2(
		type *r, int allow_e)
{
	r = type_skip_attrs_casts(r);
	if(r->type == type_btype){
		const btype *t = r->bits.type;
		switch(t->primitive){
			case type_enum:
				if(!allow_e)
			default:
					break;
			case type_struct:
			case type_union:
				return t->sue;
		}
	}
	return NULL;
}

/* newdecl_context:
 * struct B { int b; };
 * {
 *   struct A { struct B; }; // this is not a new B
 * };
 */
static type *parse_type_sue(
		enum type_primitive prim,
		int newdecl_context)
{
	int is_complete = 0;
	char *spel = NULL;
	sue_member **members = NULL;
	attribute *this_sue_attr = NULL;

	parse_add_attr(&this_sue_attr); /* struct __attr__(()) name { ... } ... */

	if(curtok == token_identifier){
		spel = token_current_spel();
		EAT(token_identifier);
	}

	/* FIXME: struct A { int i; };
	 * struct A __attr__((packed)) a; - affects all struct A instances */
	parse_add_attr(&this_sue_attr); /* int/struct-A __attr__ */

	if(accept(token_open_block)){
		if(prim == type_enum){
			for(;;){
				where w;
				expr *e;
				char *sp;
				attribute *en_attr = NULL;

				where_cc1_current(&w);
				sp = token_current_spel();
				EAT(token_identifier);

				parse_add_attr(&en_attr);

				if(accept(token_assign))
					e = parse_expr_no_comma(); /* no commas */
				else
					e = NULL;

				enum_vals_add(&members, &w, sp, e, en_attr);

				if(!accept(token_comma))
					break;

				if(curtok != token_identifier){
					if(cc1_std < STD_C99)
						warn_at(NULL, "trailing comma in enum definition");
					break;
				}
			}

		}else{
			/* always allow nameless structs (C11)
			 * we don't allow tagged ones unless
			 * -fms-extensions or -fplan9-extensions
			 */
			decl **dmembers = NULL;
			decl **i;

			parse_decls_multi_type(
					  DECL_MULTI_CAN_DEFAULT
					| DECL_MULTI_ACCEPT_FIELD_WIDTH
					| DECL_MULTI_NAMELESS
					| DECL_MULTI_ALLOW_ALIGNAS,
					/*newdecl_context:*/0,
					NULL,
					&dmembers);

			if(!dmembers){
				warn_at(NULL, "empty %s", sue_str_type(prim));
			}else{
				for(i = dmembers; *i; i++)
					dynarray_add(&members,
							sue_member_from_decl(*i));

				dynarray_free(decl **, &dmembers, NULL);
			}
		}
		EAT(token_close_block);

		is_complete = 1;

	}else if(!spel){
		die_at(NULL, "expected: %s definition or name", sue_str_type(prim));
	}

	{
		/* struct [tag] <name | '{' | ';'>
		 *
		 * it's a straight declaration if we have a ';'
		 */
		struct_union_enum_st *sue = sue_decl(
				current_scope, spel,
				members, prim, is_complete,
				/* isdef = */newdecl_context && curtok == token_semicolon);

		parse_add_attr(&this_sue_attr); /* struct A { ... } __attr__ */

		/* sue may already exist */
		attribute_append(&sue->attr, this_sue_attr);

		return type_nav_suetype(cc1_type_nav, sue);
	}
}

#include "parse_attr.c"
#include "parse_init.c"

void parse_add_attr(attribute **append)
{
	while(accept(token_attribute)){
		EAT(token_open_paren);
		EAT(token_open_paren);

		if(curtok != token_close_paren)
			attribute_append(append, parse_attr());

		EAT(token_close_paren);
		EAT(token_close_paren);
	}
}

static decl *parse_at_tdef(void)
{
	if(curtok == token_identifier){
		decl *d = symtab_search_d(current_scope, token_current_spel_peek(), NULL);

		if(d && d->store == store_typedef)
			return d;
	}
	return NULL;
}

int parse_at_decl(void)
{
	/* this is similar to parse_btype() initial test logic */
	switch(curtok){
		default:
			return curtok_is_type_qual()
				|| curtok_is_decl_store()
				|| curtok_is_type_primitive();

		case token_signed:
		case token_unsigned:
		case token_inline:
		case token__Noreturn:
		case token_struct:
		case token_union:
		case token_enum:
		case token_typeof:
		case token___builtin_va_list:
		case token_attribute:
		case token__Alignas:
			return 1;

		case token_identifier:
			return !!parse_at_tdef();
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
	if(!store)
		die_at(NULL, "storage unwanted (%s)", decl_store_to_str(st));

	if(*pstore_set)
		die_at(NULL, "second store %s", decl_store_to_str(st));

	*store = st;
	*pstore_set = 1;
}

static type *parse_btype(
		enum decl_storage *store, struct decl_align **palign,
		int newdecl_context)
{
	/* *store and *palign should be initialised */
	expr *tdef_typeof = NULL;
	attribute *attr = NULL;
	enum type_qualifier qual = qual_none;
	enum type_primitive primitive = type_int;
	int is_signed = 1, is_inline = 0, had_attr = 0, is_noreturn = 0, is_va_list = 0;
	int store_set = 0, signed_set = 0;
	decl *tdef_decl = NULL;
	enum
	{
		NONE,
		PRIMITIVE_MAYBE_MORE,
		PRIMITIVE_NO_MORE,
		TYPEDEF,
		TYPEOF
	} primitive_mode = NONE;

	for(;;){
		decl *tdef_decl_test;

		if(curtok_is_type_qual()){
			qual |= curtok_to_type_qualifier();
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
					die_at(NULL, "type primitive (%s) with %s",
							type_primitive_to_str(primitive),
							primitive_mode == TYPEDEF ? "typedef-instance" : "typeof");
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

			EAT(curtok);

			switch(tok){
#define CASE(a)                              \
				case token_ ## a:                    \
					tref = parse_type_sue(type_ ## a,  \
							newdecl_context);              \
					str = #a;                          \
					break

				CASE(enum);
				CASE(struct);
				CASE(union);

				default:
					ICE("wat");
			}

			if(signed_set || primitive_mode != NONE)
				die_at(NULL, "primitive/signed/unsigned with %s", str);

			/* fine... although a _Noreturn function returning a sue
			 * is pretty daft... */
			if(is_noreturn)
				tref = type_attributed(tref, attribute_new(attr_noreturn));

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
			return type_qualify(tref, qual);

		}else if(accept(token_typeof)){
			if(primitive_mode != NONE)
				die_at(NULL, "typeof specifier after primitive");

			tdef_typeof = parse_expr_sizeof_typeof_alignof(what_typeof);
			primitive_mode = TYPEOF;

		}else if(accept(token___builtin_va_list)){
			if(primitive_mode != NONE)
				die_at(NULL, "can't combine previous primitive with va_list");

			primitive_mode = PRIMITIVE_NO_MORE;
			is_va_list = 1;
			primitive = type_struct;

		}else if(!signed_set /* can't sign a typedef */
		&& (tdef_decl_test = parse_at_tdef()))
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

			assert(tdef_decl_test->store == store_typedef);

			tdef_decl = tdef_decl_test;
			tdef_typeof = expr_new_sizeof_type(tdef_decl->ref, what_typeof);

			primitive_mode = TYPEDEF;

			EAT(token_identifier);

		}else if(curtok == token_attribute){
			parse_add_attr(&attr); /* __attr__ int ... */
			had_attr = 1;
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

			if((as_ty = parse_type(newdecl_context))){
				da->as_int = 0;
				da->bits.align_ty = as_ty;
			}else{
				da->as_int = 1;
				da->bits.align_intk = parse_expr_exp();
			}

			EAT(token_close_paren);

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
	|| had_attr
	|| is_noreturn
	|| (palign && *palign))
	{
		type *r;

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
			r = type_nav_va_list(cc1_type_nav);

		}else switch(primitive_mode){
			case TYPEDEF:
			case TYPEOF:
				UCC_ASSERT(tdef_typeof, "no tdef_typeof for typedef/typeof");
				/* signed size_t x; */
				if(signed_set){
					die_at(NULL, "signed/unsigned not allowed with typedef instance (%s)",
							tdef_typeof->bits.ident.spel);
				}

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
		&& (*store & STORE_MASK_STORE) == store_typedef
		&& palign && *palign)
		{
			die_at(NULL, "typedefs can't be aligned");
		}

		r = type_qualify(r, qual);

		if(is_noreturn)
			attribute_append(&attr, attribute_new(attr_noreturn));

		parse_add_attr(&attr); /* int/struct-A __attr__ */
		r = type_attributed(r, attr);

		return r;
	}else{
		return NULL;
	}
}

static int parse_curtok_is_type(void)
{
	if(curtok_is_type_qual()
	|| curtok_is_decl_store()
	|| curtok_is_type_primitive())
		return 1;

	switch(curtok){
		case token_signed:
		case token_unsigned:
		case token_struct:
		case token_union:
		case token_enum:
		case token_typeof:
		case token_attribute:
		case token___builtin_va_list:
			return 1;

		case token_identifier:
			return typedef_visible(current_scope, token_current_spel_peek());

		default:
			break;
	}

	return 0;
}

static decl *parse_arg_decl(void)
{
	/* argument decls can default to int */
	const enum decl_mode flags = DECL_CAN_DEFAULT | DECL_ALLOW_STORE;
	decl *argdecl = parse_decl_single(flags, 0);

	if(!argdecl)
		die_at(NULL, "type expected (got %s)", token_to_str(curtok));

	switch(argdecl->store & STORE_MASK_STORE){
		default:
			parse_had_error = 1;
			warn_at_print_error(NULL, "%s storage on parameter",
					decl_store_to_str(argdecl->store));
			break;

		case store_default:/* aka: no store */
		case store_register:
			break;
	}

	return argdecl;
}

funcargs *parse_func_arglist()
{
	funcargs *args = funcargs_new();

	if(curtok == token_close_paren)
		goto empty_func;

	/* we allow default-to-int here, but need to make
	 * sure we also handle old functions.
	 *
	 * if we have an ident that isn't a typedef, it's an old-func
	 *
	 * f( <here>  (int)) = f(int (int)) = f(int (*)(int))
	 * f( <here> ident) -> old function
	 */
	if(curtok != token_identifier || parse_at_tdef()){
		decl *argdecl = parse_arg_decl();

		/* check for x(void) (or an equivalent typedef) */
		/* can't use type_is, since that requires folding */
		if(argdecl->ref->type == type_btype
		&& argdecl->ref->bits.type->primitive == type_void
		&& !argdecl->spel)
		{
			/* x(void); */
			funcargs_empty(args);
			args->args_void = 1; /* (void) vs () */
			goto fin;
		}

		for(;;){
			dynarray_add(&args->arglist, argdecl);

			if(curtok == token_close_paren)
				break;

			EAT_OR_DIE(token_comma);

			if(accept(token_elipsis)){
				args->variadic = 1;
				break;
			}

			/* continue loop */
			argdecl = parse_arg_decl();
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

			EAT(token_identifier);

			if(curtok == token_close_paren)
				break;

			EAT_OR_DIE(token_comma);
		}while(1);

		cc1_warn_at(NULL, 0, WARN_OMITTED_PARAM_TYPES,
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
	enum type_parsed_kind
	{
		PARSED_PTR,
		PARSED_FUNC,
		PARSED_ARRAY
	} type;

	attribute *attr;

	union
	{
		struct
		{
			type *(*maker)(type *);
			attribute *attr;
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
			int is_static;
		} array;
	} bits;

	type_parsed *prev;
};

static type_parsed *parsed_type_ptr(
		enum decl_mode mode, decl *dfor,
		type_parsed *base);
#define parsed_type_declarator parsed_type_ptr

static type_parsed *type_parsed_new(
		enum type_parsed_kind kind, type_parsed *prev)
{
	type_parsed *tp = umalloc(sizeof *tp);
	tp->type = kind;
	tp->prev = prev;
	return tp;
}

static type_parsed *parsed_type_nest(
		enum decl_mode mode, decl *dfor, type_parsed *base)
{
	if(accept(token_open_paren)){
		type_parsed *ret;

		/*
		 * we could be here:
		 * int (int a) - from either "^int(int...)" or "void f(int (int));"
		 *                                ^                        ^
		 * in which case, we've read the first "int", stop early, and unget the open paren
		 *
		 * we don't look for open parens - they're used for nexting, e.g.
		 * int ((*p)(void));
		 */
		if(parse_curtok_is_type() || curtok == token_close_paren){
			/* int() or char(short) - func decl */
			uneat(token_open_paren);
			/* parse_...func will grab this as funcargs instead */
			return base;
		}

		ret = parsed_type_declarator(mode, dfor, base);
		EAT(token_close_paren);
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
		enum decl_mode mode, decl *dfor, type_parsed *base)
{
	type_parsed *r = parsed_type_nest(mode, dfor, base);

	while(accept(token_open_square)){
		expr *size;
		enum type_qualifier q = qual_none;
		int is_static = 0;

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
			/* take size as zero */
			size = NULL;
		}else{
			/* fold.c checks for const-ness */
			/* grammar says it's a conditional here, hence no-comma */
			size = parse_expr_no_comma();
			EAT(token_close_square);
		}

		if(is_static > 1)
			die_at(NULL, "multiple static specifiers in array size");

		r = type_parsed_new(PARSED_ARRAY, r);
		r->bits.array.size = size;
		r->bits.array.qual = q;
		r->bits.array.is_static = is_static;
	}

	return r;
}

static type_parsed *parsed_type_func(
		enum decl_mode mode, decl *dfor, type_parsed *base)
{
	type_parsed *sub = parsed_type_array(mode, dfor, base);

	while(accept(token_open_paren)){
		current_scope = symtab_new(current_scope, where_cc1_current(NULL));

		sub = type_parsed_new(PARSED_FUNC, sub);
		sub->bits.func.arglist = parse_func_arglist();
		sub->bits.func.scope = current_scope;

		current_scope = current_scope->parent;

		EAT(token_close_paren);
	}

	return sub;
}

static type_parsed *parsed_type_ptr(
		enum decl_mode mode, decl *dfor, type_parsed *base)
{
	int ptr;

	if((ptr = accept(token_multiply)) || accept(token_xor)){
		type *(*maker)(type *) = ptr ? type_ptr_to : type_block_of;

		type_parsed *r_ptr;
		attribute *attr = NULL;

		enum type_qualifier qual = qual_none;

		while(curtok_is_type_qual() || curtok == token_attribute){
			if(curtok == token_attribute){
				parse_add_attr(&attr);
			}else{
				qual |= curtok_to_type_qualifier();
				EAT(curtok);
			}
		}

		r_ptr = type_parsed_new(PARSED_PTR, base);
		r_ptr->bits.ptr.maker = maker;
		r_ptr->bits.ptr.attr = attr;
		r_ptr->bits.ptr.qual = qual;

		return parsed_type_declarator(mode, dfor, r_ptr);
	}else{
		return parsed_type_func(mode, dfor, base);
	}
}

static type *parse_type_declarator(
		enum decl_mode mode, decl *dfor, type *base)
{
	type_parsed *parsed = parsed_type_declarator(mode, dfor, NULL);
	type_parsed *i;
	type *ty = base;

	for(i = parsed; i; i = i->prev){
		switch(i->type){
			case PARSED_PTR:
				ty = type_qualify(
						i->bits.ptr.maker(ty),
						i->bits.ptr.qual);
				break;
			case PARSED_ARRAY:
				ty = type_qualify(
						type_array_of_static(
							ty,
							i->bits.array.size,
							i->bits.array.is_static),
						i->bits.ptr.qual);
				break;
			case PARSED_FUNC:
				ty = type_func_of(
						ty,
						i->bits.func.arglist,
						i->bits.func.scope);
				break;
		}

		ty = type_attributed(ty, i->attr);
	}

	return ty;
}

type *parse_type(int newdecl)
{
	type *btype = parse_btype(NULL, NULL, newdecl);

	return btype ? parse_type_declarator(0, NULL, btype) : NULL;
}

static void parse_add_asm(decl *d)
{
	if(accept(token_asm)){
		char *rename, *p;

		EAT(token_open_paren);

		if(curtok != token_string)
			die_at(NULL, "string expected");

		token_get_current_str(&rename, NULL, NULL, NULL);
		EAT(token_string);

		EAT(token_close_paren);

		/* only allow [0-9A-Za-z_.] */
		for(p = rename; *p; p++)
			if(!isalnum(*p) && *p != '_' && *p != '.'){
				warn_at(NULL, "asm name contains character 0x%x", *p);
				break;
			}

		d->spel_asm = rename;
	}
}

static decl *parse_decl(type *btype, enum decl_mode mode)
{
	decl *d = decl_new();
	where w_eq;

	d->ref = parse_type_declarator(mode, d, btype);

	/* only check if it's not a function, otherwise it could be
	 * int f(i)
	 *   __attribute__(()) int i;
	 * {
	 * }
	 */

	if(!PARSE_DECL_IS_FUNC(d)){
		/* parse __asm__ naming before attributes, as per gcc and clang */
		parse_add_asm(d);
		parse_add_attr(&d->attr); /* int spel __attr__ */
	}

	if(d->spel && !PARSE_DECL_IS_FUNC(d) && accept_where(token_assign, &w_eq)){
		d->bits.var.init = parse_initialisation();
		/* top-level inits have their .where on the '=' token */
		memcpy_safe(&d->bits.var.init->where, &w_eq);
	}

	return d;
}

static void prevent_typedef(enum decl_storage store)
{
	if(store_typedef == store)
		die_at(NULL, "typedef unexpected");
}

static type *default_type(void)
{
	cc1_warn_at(NULL, 0, WARN_IMPLICIT_INT, "defaulting type to int");

	return type_nav_btype(cc1_type_nav, type_int);
}

static decl *parse_decl_extra(
		type *r, enum decl_mode mode,
		enum decl_storage store, struct decl_align *align)
{
	decl *d = parse_decl(r, mode);
	d->store = store;

	if(!PARSE_DECL_IS_FUNC(d))
		d->bits.var.align = align;
	else if(align)
		ICE("align for function?");

	return d;
}

decl *parse_decl_single(enum decl_mode mode, int newdecl)
{
	enum decl_storage store = store_default;
	type *r = parse_btype(
			mode & DECL_ALLOW_STORE ? &store : NULL,
			/*align:*/NULL,
			newdecl);

	if(!r){
		if((mode & DECL_CAN_DEFAULT) == 0)
			return NULL;

		r = default_type();

	}else{
		prevent_typedef(store);
	}

	return parse_decl_extra(r, mode, store, NULL /* align */);
}

decl **parse_decls_one_type(int newdecl)
{
	enum decl_storage store = store_default;
	struct decl_align *align = NULL;
	type *r = parse_btype(&store, &align, newdecl);
	decl **decls = NULL;

	if(!r)
		return NULL;

	prevent_typedef(store);

	do{
		decl *d = parse_decl_extra(r, DECL_SPEL_NEED, store, align);
		dynarray_add(&decls, d);
	}while(accept(token_comma));

	return decls;
}

static int is_old_func(decl *d)
{
	type *r = PARSE_type_is(d->ref, type_func);
	return r && r->bits.func.args->args_old_proto;
}

static void check_and_replace_old_func(decl *d, decl **old_args)
{
	/* check then replace old args */
	int n_proto_decls, n_old_args;
	int i;
	funcargs *dfuncargs = d->ref->bits.func.args;

	UCC_ASSERT(PARSE_type_is(d->ref, type_func), "not func");

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

		if(DECL_IS_FUNC(old_args[i]) || !old_args[i]->bits.var.init)
			die_at(&old_args[i]->where, "parameter \"%s\" is initialised", old_args[i]->spel);

		for(j = 0; j < n_proto_decls; j++){
			if(!strcmp(old_args[i]->spel, dfuncargs->arglist[j]->spel)){

				/* replace the old implicit int arg's type
				 *
				 * note we don't replace the decl itself, since that's
				 * now in an argument symtable somewhere in scope too
				 */
				decl_replace_with(dfuncargs->arglist[j], old_args[i]);

				decl_free(old_args[i]);

				found = 1;
				break;
			}
		}

		if(!found)
			die_at(&old_args[i]->where, "no such parameter '%s'", old_args[i]->spel);
	}
}

static void decl_pull_to_func(decl *const d_this, decl *const d_prev)
{
	char wbuf[WHERE_BUF_SIZ];

	if(!type_is(d_prev->ref, type_func))
		return; /* error caught later */

	if(d_prev->bits.func.code){
		/* just warn that we have a declaration
		 * after a definition, then get out.
		 *
		 * any declarations after this aren't warned about
		 * (since d_prev is different), but one warning is fine
		 */
		warn_at(&d_this->where,
				"declaration of \"%s\" after definition is ignored\n"
				"%s: note: definition here",
				d_this->spel,
				where_str_r(wbuf, &d_prev->where));
		return;
	}

	if(d_this->spel_asm){
		if(d_prev->spel_asm){
			parse_had_error = 1;
			warn_at_print_error(&d_this->where,
					"second asm() rename on \"%s\"\n"
					"%s: note: previous definition here",
					d_this->spel, where_str_r(wbuf, &d_prev->where));
		}
	}else if(d_prev->spel_asm){
		d_this->spel_asm = d_prev->spel_asm;
	}

	/* propagate static and inline */
	if((d_prev->store & STORE_MASK_STORE) == store_static)
		d_this->store = (d_this->store & ~STORE_MASK_STORE) | store_static;

	d_this->store |= (d_prev->store & STORE_MASK_EXTRA);

	/* update the f(void) bools */
	{
		funcargs *fargs_this = type_funcargs(d_this->ref),
		         *fargs_prev = type_funcargs(d_prev->ref);

		fargs_this->args_void          |= fargs_prev->args_void;
		fargs_this->args_void_implicit |= fargs_prev->args_void_implicit;
	}
}

int parse_decls_single_type(
		enum decl_multi_mode mode,
		int newdecl,
		symtable *scope,
		decl ***pdecls)
{
	const enum decl_mode parse_flag =
		(mode & DECL_MULTI_CAN_DEFAULT ? DECL_CAN_DEFAULT : 0);

	enum decl_storage store = store_default;
	struct decl_align *align = NULL;
	type *this_ref;
	decl *last = NULL;

	UCC_ASSERT(scope || pdecls, "what shall I do?");

	parse_static_assert();

	this_ref = parse_btype(
			mode & DECL_MULTI_ALLOW_STORE ? &store : NULL,
			mode & DECL_MULTI_ALLOW_ALIGNAS ? &align : NULL,
			newdecl);

	if(!this_ref){
		/* can_default makes sure we don't parse { int *p; *p = 5; } the latter as a decl */
		if(parse_at_decl_spec() && (mode & DECL_MULTI_CAN_DEFAULT)){
			this_ref = default_type();
		}else{
			return 0; /* normal exit */
		}
	}

	if(store != store_typedef){
		struct_union_enum_st *sue = PARSE_type_is_s_or_u(this_ref);

		if(sue && !parse_at_decl_spec()){
			/*
			 * struct { int i; }; - continue to next one
			 * this is either struct or union, not enum
			 */
			if((mode & DECL_MULTI_NAMELESS) == 0){
				enum type_qualifier qual;

				if(sue->anon)
					warn_at(NULL, "anonymous %s with no instances", sue_str(sue));

				/* check for storage/qual on no-instance */
				qual = type_qual(this_ref);
				if(qual || store != store_default){
					warn_at(NULL, "ignoring %s%s%son no-instance %s",
							store != store_default ? decl_store_to_str(store) : "",
							store != store_default ? " " : "",
							type_qual_to_str(qual, 1),
							sue_str(sue));
				}

				goto next;
			}
		}
	}

	do{
		decl *d = parse_decl_extra(this_ref, parse_flag, store, align);
		int had_field_width = 0;

		if((mode & DECL_MULTI_ACCEPT_FIELD_WIDTH)
		&& !PARSE_DECL_IS_FUNC(d)
		&& accept(token_colon))
		{
			/* normal decl, check field spec */
			d->bits.var.field_width = parse_expr_no_comma();
			had_field_width = 1;
		}

		if(!d->spel && !had_field_width){
			/*
			 * int; - fine for "int;", but "int i,;" needs to fail
			 * struct A; - fine
			 * struct { int i; }; - warn
			 */

			if(last == NULL){
				int warn = 0;
				struct_union_enum_st *sue;

				/* check for no-fwd and anon */
				sue = type_is_s_or_u_or_e(this_ref);
				switch(sue ? sue->primitive : type_unknown){
					case type_struct:
					case type_union:
						/* don't warn for tagged struct/unions */
						if(mode & DECL_MULTI_NAMELESS)
							goto add;

						UCC_ASSERT(!sue->anon, "tagless struct should've been caught above");
					case type_enum:
						warn = 0; /* don't warn for enums - they're always declarations */
						break;

					default:
						warn = 1;
				}
				if(warn){
					/*
					 * die if it's a complex decl,
					 * e.g. int (const *a)
					 * [function with int argument, not a pointer to const int
					 */
					const char *emsg = "declaration doesn't declare anything";

					if(PARSE_type_is(d->ref, type_btype)
					|| PARSE_type_is(d->ref, type_tdef))
					{
						warn_at(&d->where, "%s", emsg);
					}else{
						warn_at_print_error(&d->where, "%s", emsg);
						parse_had_error = 1;
					}
				}

				decl_free(d);
				goto next;
			}
			die_at(&d->where, "identifier expected after decl (got %s)", token_to_str(curtok));
		}else if(PARSE_DECL_IS_FUNC(d)){
			int need_func = 0;

			/* special case - support asm directly after a function
			 * no parse ambiguity - asm can only appear at the end of a decl,
			 * before __attribute__
			 */
			if(curtok == token_asm)
				parse_add_asm(d);

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
				attribute *a = NULL;
				parse_add_attr(&a);
				d->ref = type_attributed(d->ref, a);
			}else{
				decl **old_args = NULL;
				/* NULL - we don't want these in a scope */
				parse_decls_multi_type(0, /*newdecl_context:*/0, NULL, &old_args);
				if(old_args){
					check_and_replace_old_func(d, old_args);

					dynarray_free(decl **, &old_args, NULL);

					/* old function with decls after the close paren,
					 * need a function */
					need_func = 1;
				}
			}

			/* clang-style allows __attribute__ and then a function block */
			if(need_func || curtok == token_open_block){
				type *func_r = PARSE_DECL_IS_FUNC(d);
				symtable *const old_scope = current_scope;

				/* need to set scope to include function argumen
				 * e.g. f(struct A { ... })
				 */
				UCC_ASSERT(func_r, "function expected");
				current_scope = func_r->bits.func.arg_scope;

				d->bits.func.code = parse_stmt_block();

				current_scope = current_scope->parent;
				UCC_ASSERT(current_scope == old_scope,
						"scope change in parsing func block");

				/* if:
				 * f(){...}, then we don't have args_void, but implicitly we do
				 */
				type_funcargs(d->ref)->args_void_implicit = 1;
			}
		}

add:
		if(d->spel){
			/* Look for a previous declaration of d->spel.
			 * if found, we pull its asm() and attributes to the current,
			 * thus propagating them down in O(1) to the eventual definition.
			 * Do this before adding, so we don't find 'd'
			 *
			 * This also means any use of d will have the most up to date
			 * attribute information about it
			 */
			decl *d_prev = symtab_search_d(current_scope, d->spel, NULL);

			if(d_prev){
				/* link the proto chain for __attribute__ checking,
				 * nested function prototype checking and
				 * '.extern fn' code gen easing
				 */
				d->proto = d_prev;

				if(PARSE_DECL_IS_FUNC(d) && PARSE_DECL_IS_FUNC(d_prev))
					decl_pull_to_func(d, d_prev);
			}
		}

		if(scope)
			dynarray_add(&scope->decls, d);
		if(pdecls)
			dynarray_add(pdecls, d);

		/* FIXME: check later for functions, not here - typedefs */
		if(PARSE_DECL_IS_FUNC(d)){
			if(d->bits.func.code && (mode & DECL_MULTI_ACCEPT_FUNC_CODE) == 0)
				die_at(&d->where, "function code not wanted (%s)", d->spel);

			if((mode & DECL_MULTI_ACCEPT_FUNC_DECL) == 0)
				die_at(&d->where, "function decl not wanted (%s)", d->spel);
		}

		if(store == store_typedef){
			if(PARSE_DECL_IS_FUNC(d)){
				if(d->bits.func.code)
					die_at(&d->where, "can't have a typedef function with code");
			}else if(d->bits.var.init){
				die_at(&d->where, "can't init a typedef");
			}
		}

		last = d;
	}while(accept(token_comma));


	if(last && (!PARSE_DECL_IS_FUNC(last) || !last->bits.func.code)){
next:
		/* end of type, if we have an identifier, '(' or '*', it's an unknown type name */
		if(parse_at_decl_spec())
			die_at(NULL, "unknown type name '%s'", last->spel);
		/* else die here: */
		EAT(token_semicolon);
	}

	return 1;
}

void parse_decls_multi_type(
		enum decl_multi_mode mode,
		int newdecl_context,
		symtable *scope,
		decl ***pdecls)
{
	for(;;)
		if(!parse_decls_single_type(mode,
					newdecl_context, scope, pdecls))
			break;
}
