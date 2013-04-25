#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "typedef.h"
#include "decl_init.h"
#include "funcargs.h"

#include "tokenise.h"
#include "tokconv.h"

#include "sue.h"
#include "sym.h"

#include "cc1.h"

#include "parse.h"
#include "parse_type.h"

#include "expr.h"

/*#define PARSE_DECL_VERBOSE*/

/* we don't do the type_ref_is_* since it needs to be folded for that */
#define PARSE_type_ref_is(r, ty) ((type_ref_skip_casts(r)->type == ty) ? (r) : NULL)
#define PARSE_DECL_IS_FUNC(d) PARSE_type_ref_is(type_ref_skip_casts(d->ref), type_ref_func)

#define PARSE_type_ref_is_s_or_u_or_e(r) PARSE_type_ref_is_s_or_u_or_e2(r, 1)
#define PARSE_type_ref_is_s_or_u(r)      PARSE_type_ref_is_s_or_u_or_e2(r, 0)

struct_union_enum_st *PARSE_type_ref_is_s_or_u_or_e2(type_ref *r, int allow_e)
{
	r = type_ref_skip_casts(r);
	if(r->type == type_ref_type){
		type *t = r->bits.type;
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

static void parse_add_attr(decl_attr **append);
static type_ref *parse_type_ref2(enum decl_mode mode, char **sp);

void parse_sue_preamble(type **tp, char **psp, enum type_primitive primitive)
{
	char *spel;
	type *t;

	spel = NULL;
	t = type_new_primitive(primitive);

	if(curtok == token_identifier){
		spel = token_current_spel();
		EAT(token_identifier);
	}

	parse_add_attr(&t->attr); /* int/struct-A __attr__ */

	*psp = spel;
	*tp = t;
}

/* sue = struct/union/enum */
type *parse_type_sue(enum type_primitive prim)
{
	type *t;
	char *spel;
	sue_member **members;
	int is_complete = 0;

	parse_sue_preamble(&t, &spel, prim);

	members = NULL;

	if(accept(token_open_block)){
		if(prim == type_enum){
			for(;;){
				expr *e;
				char *sp;

				sp = token_current_spel();
				EAT(token_identifier);

				if(accept(token_assign))
					e = parse_expr_no_comma(); /* no commas */
				else
					e = NULL;

				enum_vals_add(&members, sp, e);

				if(!accept(token_comma))
					break;

				if(curtok != token_identifier){
					if(cc1_std < STD_C99)
						WARN_AT(NULL, "trailing comma in enum definition");
					break;
				}
			}

		}else{
			/* always allow nameless structs (C11)
			 * we don't allow tagged ones unless
			 * -fms-extensions or -fplan9-extensions
			 */
			decl **dmembers = parse_decls_multi_type(
					DECL_MULTI_CAN_DEFAULT |
					DECL_MULTI_ACCEPT_FIELD_WIDTH |
					DECL_MULTI_NAMELESS);
			decl **i;

			if(!dmembers){
				const char *t = sue_str_type(prim);

				if(curtok == token_colon)
					DIE_AT(NULL, "can't have initial %s padding", t);

				WARN_AT(NULL, "empty %s", t);

			}else{
				for(i = dmembers; i && *i; i++){
					sue_member *sm = umalloc(sizeof *sm);
					sm->struct_member = *i;
					dynarray_add(&members, sm);
				}

				dynarray_free(&dmembers, NULL);
			}
		}
		EAT(token_close_block);

		is_complete = 1;

	}else if(!spel){
		DIE_AT(NULL, "expected: %s definition or name", sue_str_type(prim));

	}else{
		/* predeclaring */
		if(prim == type_enum && !sue_find_this_scope(current_scope, spel))
			cc1_warn_at(NULL, 0, 1, WARN_PREDECL_ENUM, "predeclaration of enums is not C99");
	}

	t->sue = sue_find_or_add(current_scope, spel, members, prim, is_complete);

	parse_add_attr(&t->sue->attr); /* struct A {} __attr__ */

	return t;
}

#include "parse_attr.c"
#include "parse_init.c"

static void parse_add_attr(decl_attr **append)
{
	while(accept(token_attribute)){
		EAT(token_open_paren);
		EAT(token_open_paren);

		if(curtok != token_close_paren)
			decl_attr_append(append, parse_attr());

		EAT(token_close_paren);
		EAT(token_close_paren);
	}
}

static type_ref *parse_btype(enum decl_storage *store)
{
	expr *tdef_typeof = NULL;
	decl_attr *attr = NULL;
	enum type_qualifier qual = qual_none;
	enum type_primitive primitive = type_int;
	int is_signed = 1, is_inline = 0, had_attr = 0, is_noreturn = 0;
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

	if(store)
		*store = store_default;

	for(;;){
		decl *tdef_decl_test;

		if(curtok_is_type_qual()){
			qual |= curtok_to_type_qualifier();
			EAT(curtok);

		}else if(curtok_is_decl_store()){
			const enum decl_storage st = curtok_to_decl_storage();

			if(!store)
				DIE_AT(NULL, "storage unwanted (%s)", decl_store_to_str(st));

			if(store_set)
				DIE_AT(NULL, "second store %s", decl_store_to_str(st));

			*store = st;
			store_set = 1;
			EAT(curtok);

		}else if(curtok_is_type_primitive()){
			const enum type_primitive got = curtok_to_type_primitive();

			switch(primitive_mode){
				case PRIMITIVE_MAYBE_MORE:
					/* allow "long int" and "short int" */
#define INT(x)   x == type_int
#define SHORT(x) x == type_short
#define LONG(x)  x == type_long
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
								primitive = type_llong, die = 0;
							else if((LONG(primitive) && DBL(got)) || (DBL(primitive) && LONG(got)))
								primitive = type_ldouble, die = 0;

							primitive_mode = PRIMITIVE_NO_MORE;
						}

						if(die)
				case PRIMITIVE_NO_MORE:
							DIE_AT(NULL, "second type primitive %s", type_primitive_to_str(got));
					}
					if(primitive_mode == PRIMITIVE_MAYBE_MORE){
						switch(primitive){
							case type_int:
							case type_long:
								/* allow short int, long int and long long */
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
					DIE_AT(NULL, "type primitive (%s) with %s",
							type_primitive_to_str(primitive),
							primitive_mode == TYPEDEF ? "typedef-instance" : "typeof");
			}

			EAT(curtok);

		}else if(curtok == token_signed || curtok == token_unsigned){
			is_signed = curtok == token_signed;

			if(signed_set)
				DIE_AT(NULL, "unwanted second \"%ssigned\"", is_signed ? "" : "un");

			signed_set = 1;
			EAT(curtok);

		}else if(curtok == token_inline){
			is_inline = 1;
			EAT(curtok);

		}else if(curtok == token__Noreturn){
			is_noreturn = 1;
			EAT(curtok);

		}else if(curtok == token_struct || curtok == token_union || curtok == token_enum){
			const enum token tok = curtok;
			const char *str;
			type *t;

			EAT(curtok);

			switch(tok){
#define CASE(a)                           \
				case token_ ## a:                 \
					t = parse_type_sue(type_ ## a); \
					str = #a;                       \
					break

				CASE(enum);
				CASE(struct);
				CASE(union);

				default:
					ICE("wat");
			}

			if(signed_set || primitive_mode != NONE || is_inline)
				DIE_AT(&t->where, "primitive/signed/unsigned/inline with %s", str);

			/* fine... although a _Noreturn function returning a sue
			 * is pretty daft... */
			if(is_noreturn)
				decl_attr_append(&t->attr, decl_attr_new(attr_noreturn));

			/*
			 * struct A { ... } const x;
			 * accept qualifiers for the type, not decl
			 */
			while(curtok_is_type_qual()){
				qual |= curtok_to_type_qualifier();
				EAT(curtok);
			}

			/* *store is assigned elsewhere */
			return type_ref_new_cast_add(type_ref_new_type(t), qual);

		}else if(accept(token_typeof)){
			if(primitive_mode != NONE)
				DIE_AT(NULL, "typeof specifier after primitive");

			tdef_typeof = parse_expr_sizeof_typeof(1);
			primitive_mode = PRIMITIVE_NO_MORE;

		}else if(!signed_set /* can't sign a typedef */
		&& curtok == token_identifier
		&& (tdef_decl_test = scope_find(current_scope, token_current_spel_peek())))
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

			if(tdef_decl_test->store != store_typedef){
				/* found an identifier instead */
				tdef_decl_test = NULL;
				break;
			}

			tdef_decl = tdef_decl_test;
			tdef_typeof = expr_new_sizeof_type(tdef_decl->ref, 1);

			tdef_typeof->bits.ident.spel = token_current_spel();
			primitive_mode = PRIMITIVE_NO_MORE;

			EAT(token_identifier);

			/*
			 * FIXME
			 * check for a following colon, in the case of
			 * typedef int x;
			 * x:;
			 *
			 * x is a valid label
			 */

		}else if(curtok == token_attribute){
			parse_add_attr(&attr); /* __attr__ int ... */
			had_attr = 1;
			/*
			 * can't depend on !!attr, since it is null when:
			 * __attribute__(());
			 */

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
	|| is_noreturn)
	{
		type_ref *r;

		switch(primitive){
			case type__Bool:
			case type_float:
			case type_double:
			case type_ldouble:
				if(signed_set)
					DIE_AT(NULL, "%ssigned with _Bool", is_signed ? "" : "un");
			default:
				break;
		}

		switch(primitive_mode){
			case TYPEDEF:
			case TYPEOF:
				UCC_ASSERT(tdef_typeof, "no tdef_typeof for typedef/typeof");
				/* signed size_t x; */
				if(signed_set){
					DIE_AT(NULL, "signed/unsigned not allowed with typedef instance (%s)",
							tdef_typeof->bits.ident.spel);
				}

				r = type_ref_new_tdef(tdef_typeof, tdef_decl);
				break;

			case PRIMITIVE_NO_MORE:
			case PRIMITIVE_MAYBE_MORE:
			case NONE:
			{
				type *t = type_new_primitive(
						primitive_mode == NONE ? type_int : primitive);

				t->is_signed = is_signed;

				r = type_ref_new_type(t);
				break;
			}
		}

		r = type_ref_new_cast_add(r, qual);

		if(is_inline){
			if(store)
				*store |= store_inline;
			else
				DIE_AT(NULL, "inline not wanted");
		}

		r->attr = attr;
		parse_add_attr(&r->attr); /* int/struct-A __attr__ */

		if(is_noreturn)
			decl_attr_append(&r->attr, decl_attr_new(attr_noreturn));

		return r;
	}else{
		return NULL;
	}
}

int parse_curtok_is_type(void)
{
	if(curtok_is_type_qual() || curtok_is_decl_store() || curtok_is_type_primitive())
		return 1;

	switch(curtok){
		case token_signed:
		case token_unsigned:
		case token_struct:
		case token_union:
		case token_enum:
		case token_typeof:
		case token_attribute:
			return 1;

		case token_identifier:
			return typedef_visible(current_scope, token_current_spel_peek());

		default:
			break;
	}

	return 0;
}

funcargs *parse_func_arglist()
{
	funcargs *args;
	decl *argdecl;

	args = funcargs_new();

	if(curtok == token_close_paren)
		goto empty_func;

	argdecl = parse_decl_single(0);

	if(argdecl){

		/* check for x(void) (or an equivalent typedef) */
		/* can't use type_ref_is, since that requires folding */
		if(argdecl->ref->type == type_ref_type
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
			/* actually, we don't need a type here, default to int, i think */
			argdecl = parse_decl_single(DECL_CAN_DEFAULT);
		}

fin:;

	}else{
		do{
			decl *d = decl_new();

			if(curtok != token_identifier)
				EAT(token_identifier); /* error */

			d->ref = type_ref_new_type(type_new_primitive(type_int));

			d->spel = token_current_spel();
			dynarray_add(&args->arglist, d);

			EAT(token_identifier);

			if(curtok == token_close_paren)
				break;

			EAT_OR_DIE(token_comma);
		}while(1);
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

static type_ref *parse_type_ref_nest(enum decl_mode mode, char **sp)
{
	if(accept(token_open_paren)){
		type_ref *ret;

		/*
		 * we could be here:
		 * int (int a) - from either "^int(int...)" or "void f(int (int));"
		 *                                ^                        ^
		 * in which case, we've read the first "int", stop early, and unget the open paren
		 */
		if(parse_curtok_is_type() || curtok == token_close_paren){
			/* int() - func decl */
			uneat(token_open_paren);
			/* parse_...func will grab this as funcargs instead */
			return NULL;
		}

		ret = parse_type_ref2(mode, sp);
		EAT(token_close_paren);
		return ret;

	}else if(curtok == token_identifier){
		if(!sp)
			DIE_AT(NULL, "identifier unexpected");

		*sp = token_current_spel();

		EAT(token_identifier);

	}else if(mode & DECL_SPEL_NEED){
		DIE_AT(NULL, "need identifier for decl");
	}

	return NULL;
}

static type_ref *parse_type_ref_array(enum decl_mode mode, char **sp)
{
	type_ref *r = parse_type_ref_nest(mode, sp);

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
			size = expr_new_val(0);
			/* FIXME - incomplete, not zero */
		}else{
			/* fold.c checks for const-ness */
			size = parse_expr_exp();
			EAT(token_close_square);
		}

		if(is_static > 1)
			DIE_AT(NULL, "multiple static specifiers in array size");

		r = type_ref_new_array2(r, size, q, is_static);
	}

	return r;
}

static type_ref *parse_type_ref_func(enum decl_mode mode, char **sp)
{
	type_ref *sub = parse_type_ref_array(mode, sp);

	while(accept(token_open_paren)){
		type_ref *r_new = type_ref_new_func(sub, parse_func_arglist());

		EAT(token_close_paren);

		sub = r_new;
	}

	return sub;
}

static type_ref *parse_type_ref_ptr(enum decl_mode mode, char **sp)
{
	int ptr;

	if((ptr = accept(token_multiply)) || accept(token_xor)){
		typedef type_ref *(*ptr_creator_f)(type_ref *, enum type_qualifier);
		ptr_creator_f creater = ptr ? type_ref_new_ptr : type_ref_new_block;

		enum type_qualifier qual = qual_none;

		while(curtok_is_type_qual()){
			qual |= curtok_to_type_qualifier();
			EAT(curtok);
		}

		return creater(parse_type_ref2(mode, sp), qual);
	}

	return parse_type_ref_func(mode, sp);
}

static type_ref *parse_type_ref2(enum decl_mode mode, char **sp)
{
	return parse_type_ref_ptr(mode, sp);
}

static type_ref *type_ref_reverse(type_ref *r, type_ref *subtype)
{
	/*
	 * e.g.  int (*f)() is parsed as:
	 *   func -> ptr -> NULL
	 * swap to
	 *   ptr -> func -> subtype
	 */
	type_ref *i, *next, *prev = subtype;

	for(i = r; i; prev = i, i = next){
		next = i->ref;
		i->ref = prev;
	}

	return prev;
}

static type_ref *parse_type3(
		enum decl_mode mode, char **spel, type_ref *btype)
{
	return type_ref_reverse(parse_type_ref2(mode, spel), btype);
}

type_ref *parse_type()
{
	type_ref *btype = parse_btype(NULL);

	return btype ? parse_type3(0, NULL, btype) : NULL;
}

static void parse_add_asm(decl *d)
{
	if(accept(token_asm)){
		char *rename, *p;

		EAT(token_open_paren);

		if(curtok != token_string)
			DIE_AT(NULL, "string expected");

		token_get_current_str(&rename, NULL, NULL);
		EAT(token_string);

		EAT(token_close_paren);

		/* only allow [0-9A-Za-z_.] */
		for(p = rename; *p; p++)
			if(!isalnum(*p) && *p != '_' && *p != '.'){
				WARN_AT(NULL, "asm name contains character 0x%x", *p);
				break;
			}

		d->spel_asm = rename;
	}
}

decl *parse_decl(type_ref *btype, enum decl_mode mode)
{
	char *spel = NULL;
	decl *d = decl_new();

	d->ref = parse_type3(mode, &spel, btype);

	d->spel = spel;

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

	if(d->spel && accept(token_assign))
		d->init = parse_initialisation();

	return d;
}

static void prevent_typedef(where *w, enum decl_storage store)
{
	if(store_typedef == store)
		DIE_AT(w, "typedef unexpected");
}

static type_ref *default_type(void)
{
	cc1_warn_at(NULL, 0, 1, WARN_IMPLICIT_INT, "defaulting type to int");

	return type_ref_new_type(type_new_primitive(type_int));
}

decl *parse_decl_single(enum decl_mode mode)
{
	enum decl_storage store = store_default;
	type_ref *r = parse_btype(mode & DECL_ALLOW_STORE ? &store : NULL);

	if(!r){
		if((mode & DECL_CAN_DEFAULT) == 0)
			return NULL;

		r = default_type();

	}else{
		prevent_typedef(&r->where, store);
	}

	{
		decl *d = parse_decl(r, mode);
		d->store = store;
		return d;
	}
}

decl **parse_decls_one_type()
{
	enum decl_storage store;
	type_ref *r = parse_btype(&store);
	decl **decls = NULL;

	if(!r)
		return NULL;

	prevent_typedef(&r->where, store);

	do{
		decl *d = parse_decl(r, DECL_SPEL_NEED);
		dynarray_add(&decls, d);
	}while(accept(token_comma));

	return decls;
}

static int is_old_func(decl *d)
{
	type_ref *r = PARSE_type_ref_is(d->ref, type_ref_func);
	return r && r->bits.func->args_old_proto;
}

static void check_old_func(decl *d, decl **old_args)
{
	/* check then replace old args */
	int n_proto_decls, n_old_args;
	int i;
	funcargs *dfuncargs = d->ref->bits.func;

	UCC_ASSERT(PARSE_type_ref_is(d->ref, type_ref_func), "not func");

	if(!dfuncargs->args_old_proto){
		DIE_AT(&d->where, dfuncargs->arglist
				? "unexpected old-style decls - new style proto used"
				: "parameters specified despite empty declaration in prototype");
	}

	n_proto_decls = dynarray_count(dfuncargs->arglist);
	n_old_args = dynarray_count(old_args);

	if(n_old_args > n_proto_decls)
		DIE_AT(&d->where, "old-style function decl: too many decls");

	for(i = 0; i < n_old_args; i++)
		if(old_args[i]->init)
			DIE_AT(&old_args[i]->where, "parameter \"%s\" is initialised", old_args[i]->spel);

	for(i = 0; i < n_old_args; i++){
		int j, found = 0;

		for(j = 0; j < n_proto_decls; j++){
			if(!strcmp(old_args[i]->spel, dfuncargs->arglist[j]->spel)){
				decl **replace_this;
				decl *free_this;

				/* replace the old implicit int arg */
				replace_this = &dfuncargs->arglist[j];

				free_this = *replace_this;
				*replace_this = old_args[i];

				decl_free(free_this, 0);
				found = 1;
				break;
			}
		}

		if(!found)
			DIE_AT(&old_args[i]->where, "no such parameter '%s'", old_args[i]->spel);
	}

	free(old_args);
}

decl **parse_decls_multi_type(enum decl_multi_mode mode)
{
	const enum decl_mode parse_flag = (mode & DECL_MULTI_CAN_DEFAULT ? DECL_CAN_DEFAULT : 0);
	decl **decls = NULL;
	decl *last;
	int are_tdefs;

	/* read a type, then *spels separated by commas, then a semi colon, then repeat */
	for(;;){
		enum decl_storage store = store_default;
		type_ref *this_ref;

		last = NULL;
		are_tdefs = 0;

		parse_static_assert();

		this_ref = parse_btype(mode & DECL_MULTI_ALLOW_STORE ? &store : NULL);

		if(!this_ref){
			/* can_default makes sure we don't parse { int *p; *p = 5; } the latter as a decl */
			if(parse_possible_decl() && (mode & DECL_MULTI_CAN_DEFAULT)){
				this_ref = default_type();
			}else{
				return decls;
			}
		}

		if(store == store_typedef){
			are_tdefs = 1;
		}else{
			struct_union_enum_st *sue = PARSE_type_ref_is_s_or_u(this_ref);

			if(sue && !parse_possible_decl()){
				/*
				 * struct { int i; }; - continue to next one
				 * this is either struct or union, not enum
				 */
				if((mode & DECL_MULTI_NAMELESS) == 0){
					enum type_qualifier qual;

					if(sue->anon)
						WARN_AT(&this_ref->where, "anonymous %s with no instances", sue_str(sue));

					/* check for storage/qual on no-instance */
					qual = type_ref_qual(this_ref);
					if(qual || store != store_default){
						WARN_AT(&this_ref->where, "ignoring %s%s%son no-instance %s",
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
			decl *d = parse_decl(this_ref, parse_flag);

			d->store = store;

			if(!d->spel){
				/*
				 * int; - fine for "int;", but "int i,;" needs to fail
				 * struct A; - fine
				 * struct { int i; }; - warn
				 */

				if(last == NULL){
					int warn = 0;
					struct_union_enum_st *sue;

					/* allow "int : 5;" */
					if(curtok == token_colon)
						goto add;

					/* check for no-fwd and anon */
					sue = PARSE_type_ref_is_s_or_u_or_e(this_ref);
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
#define err_nodecl "declaration doesn't declare anything"
						if(PARSE_type_ref_is(d->ref, type_ref_type))
							WARN_AT(&d->where, err_nodecl);
						else
							DIE_AT(&d->where, err_nodecl);
#undef err_nodecl
					}

					decl_free(d, 0);
					goto next;
				}
				DIE_AT(&d->where, "identifier expected after decl (got %s)", token_to_str(curtok));
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
					/* add to .ref, since this is what is checked when the function decays to a pointer */
					parse_add_attr(&d->ref->attr);
				}else{
					decl **old_args = parse_decls_multi_type(0);
					if(old_args){
						check_old_func(d, old_args);

						/* old function with decls after the close paren,
						 * need a function */
						need_func = 1;
					}
				}

				/* clang-style allows __attribute__ and then a function block */
				if(need_func || curtok != token_semicolon){
					d->func_code = parse_stmt_block();

					/* if:
					 * f(){...}, then we don't have args_void, but implicitly we do
					 */
					type_ref_funcargs(d->ref)->args_void_implicit = 1;
				}
			}

add:
			dynarray_add(are_tdefs
					? &current_scope->typedefs
					: &decls,
					d);

			/* FIXME: check later for functions, not here - typedefs */
			if(PARSE_DECL_IS_FUNC(d)){
				if(d->func_code && (mode & DECL_MULTI_ACCEPT_FUNC_CODE) == 0)
						DIE_AT(&d->where, "function code not wanted (%s)", d->spel);

				if((mode & DECL_MULTI_ACCEPT_FUNC_DECL) == 0)
					DIE_AT(&d->where, "function decl not wanted (%s)", d->spel);
			}

			if(are_tdefs){
				if(PARSE_DECL_IS_FUNC(d) && d->func_code)
					DIE_AT(&d->where, "can't have a typedef function with code");
				else if(d->init)
					DIE_AT(&d->where, "can't init a typedef");
			}

			if((mode & DECL_MULTI_ACCEPT_FIELD_WIDTH) && accept(token_colon)){
				/* normal decl, check field spec */
#ifdef FIELD_WIDTH_TODO
				d->field_width = parse_expr_exp();
#else
				ICE("TODO: field width");
#endif
			}

			last = d;
		}while(accept(token_comma));


		if(last && !last->func_code){
next:
			/* end of type, if we have an identifier, '(' or '*', it's an unknown type name */
			if(parse_possible_decl() && last)
				DIE_AT(NULL, "unknown type name '%s'", last->spel);
			/* else die here: */
			EAT(token_semicolon);
		}

		if((mode & DECL_MULTI_ACCEPT_FIELD_WIDTH) && accept(token_colon)){
			/* padding - struct { int i; :3 } */
			ICE("TODO: struct/union(?) inter-var padding");
			/* anon-decl with field width to pad? */
		}
	}
}
