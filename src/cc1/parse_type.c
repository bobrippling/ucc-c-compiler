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

static void parse_add_attr(decl_attr **append);
static type_ref *parse_type_ref2(enum decl_mode mode, char **sp);

#define INT_TYPE(t) do{ UCC_ASSERT(!t, "got type"); t = type_new_primitive(type_int); }while(0)

void parse_type_preamble(type **tp, char **psp, enum type_primitive primitive)
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

	parse_type_preamble(&t, &spel, prim);

	members = NULL;

	if(accept(token_open_block)){
		if(prim == type_enum){
			do{
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
			}while(curtok == token_identifier);

			EAT(token_close_block);
		}else{
			decl **dmembers = parse_decls_multi_type(DECL_MULTI_CAN_DEFAULT | DECL_MULTI_ACCEPT_FIELD_WIDTH);
			decl **i;

			if(!dmembers){
				if(curtok == token_colon)
					DIE_AT(NULL, "can't have initial struct padding");
				DIE_AT(NULL, "no members in struct");
			}

			for(i = dmembers; *i; i++){
				sue_member *sm = umalloc(sizeof *sm);
				sm->struct_member = *i;
				dynarray_add((void ***)&members, sm);
			}

			dynarray_free((void ***)&dmembers, NULL);

			EAT(token_close_block);
		}

	}else if(!spel){
		DIE_AT(NULL, "expected: struct definition or name");

	}else{
		/* predeclaring */
		if(prim == type_enum && !sue_find(current_scope, spel))
			cc1_warn_at(NULL, 0, 1, WARN_PREDECL_ENUM, "predeclaration of enums is not C99");
	}

	t->sue = sue_add(current_scope, spel, members, prim);

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

type_ref *parse_type(enum decl_storage *store)
{
#define PRIMITIVE_NO_MORE 2

	expr *tdef_typeof = NULL;
	decl_attr *attr = NULL;
	enum type_qualifier qual = qual_none;
	enum type_primitive primitive = type_int;
	int is_signed = 1, is_inline = 0, had_attr = 0, is_noreturn = 0;
	int store_set = 0, primitive_set = 0, signed_set = 0;
	decl *tdef_decl = NULL;

	if(store)
		*store = store_default;

	for(;;){
		decl *tdef_decl_test;

		if(curtok_is_type_qual()){
			qual |= curtok_to_type_qualifier();
			EAT(curtok);

		}else if(curtok_is_decl_store()){
			if(!store)
				DIE_AT(NULL, "storage unwanted (%s)", decl_store_to_str(*store));

			*store = curtok_to_decl_storage();

			if(store_set)
				DIE_AT(NULL, "second store %s", decl_store_to_str(*store));

			store_set = 1;
			EAT(curtok);

		}else if(curtok_is_type_primitive()){
			const enum type_primitive got = curtok_to_type_primitive();

			if(primitive_set){
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

					if(primitive_set < PRIMITIVE_NO_MORE){
						/* special case for long long and long double */
						if(LONG(primitive) && LONG(got))
							primitive = type_llong, die = 0;
						else if((LONG(primitive) && DBL(got)) || (DBL(primitive) && LONG(got)))
							primitive = type_ldouble, die = 0;
					}

					if(die)
						DIE_AT(NULL, "second type primitive %s", type_primitive_to_str(got));
				}
			}else{
				primitive = got;
			}

			primitive_set++;
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

			if(signed_set || primitive_set || is_noreturn || is_inline)
				DIE_AT(&t->where, "primitive/signed/unsigned/noreturn/inline with %s", str);

			/*
			 * struct A { ... } const x;
			 * accept qualifiers for the type, not decl
			 */
			while(curtok_is_type_qual()){
				qual |= curtok_to_type_qualifier();
				EAT(curtok);
			}

			t->qual  = qual;

			return type_ref_new_type(t);

		}else if(accept(token_typeof)){
			if(primitive_set)
				DIE_AT(NULL, "duplicate typeof specifier");

			tdef_typeof = parse_expr_sizeof_typeof(1);
			primitive_set = 1;

		}else if(curtok == token_identifier
		&& (tdef_decl_test = typedef_find(current_scope, token_current_spel_peek()))){
			/* typedef name */

			/*
			 * FIXME
			 * check for a following colon, in the case of
			 * typedef int x;
			 * x:;
			 *
			 * x is a valid label
			 */

			if(primitive_set){
				/* "int x" - we are at x, which is also a typedef somewhere */
				DIE_AT(NULL, "redefinition of %s as different symbol", token_current_spel_peek());
				break;
			}

			/*if(tdef_typeof) - can't reach due to primitive_set */

			tdef_decl = tdef_decl_test;
			tdef_typeof = expr_new_sizeof_type(tdef_decl->ref, 1);

			token_get_current_str(&tdef_typeof->spel, NULL);
			primitive_set = PRIMITIVE_NO_MORE;

			EAT(token_identifier);

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
	|| primitive_set
	|| signed_set
	|| tdef_typeof
	|| is_inline
	|| had_attr
	|| is_noreturn)
	{
		type_ref *r;

		if(tdef_typeof){
			/* signed size_t x; */
			if(signed_set){
				DIE_AT(NULL, "signed/unsigned not allowed with typedef instance (%s)",
						tdef_typeof->spel);
			}

			r = type_ref_new_tdef(tdef_typeof, tdef_decl);

		}else{
			type *t = type_new_primitive(primitive_set ? primitive : type_int);

			t->is_signed = is_signed;
			t->qual  = qual;

			r = type_ref_new_type(t);
		}

		if(is_inline)
			*store |= store_inline;

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
			return !!typedef_find(current_scope, token_current_spel_peek());

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

		/* check for x(void) */
		if( argdecl->ref->type == type_ref_type
		&&  argdecl->ref->bits.type->primitive == type_void
		&& !argdecl->spel)
		{
			/* x(void); */
			funcargs_empty(args);
			args->args_void = 1; /* (void) vs () */
			goto fin;
		}

		for(;;){
			dynarray_add((void ***)&args->arglist, argdecl);

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
			dynarray_add((void ***)&args->arglist, d);

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

type_ref *parse_type_ref_ptr(enum decl_mode mode, char **sp)
{
	int ptr;

	if((ptr = accept(token_multiply)) || accept(token_xor)){
		enum type_qualifier qual = qual_none;
		while(curtok_is_type_qual()){
			qual |= curtok_to_type_qualifier();
			EAT(curtok);
		}

		return (ptr ? type_ref_new_ptr : type_ref_new_block)(parse_type_ref2(mode, sp), qual);

	}else if(accept(token_open_paren)){
		type_ref *ret;

		/*
		 * we could be here:
		 * int (int a) - from either "^int(int...)" or "void f(int (int));"
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

type_ref *parse_type_ref_array(enum decl_mode mode, char **sp)
{
	type_ref *r = parse_type_ref_ptr(mode, sp);

	while(accept(token_open_square)){
		type_ref *r_new;
		expr *size;

		if(accept(token_close_square)){
			/* take size as zero */
			size = expr_new_val(0);
			/* FIXME - incomplete, not zero */
		}else{
			/* fold.c checks for const-ness */
			size = parse_expr_exp();
			EAT(token_close_square);
		}

		r_new = type_ref_new_array(r, size);

		r = r_new;
	}

	return r;
}

type_ref *parse_type_ref2(enum decl_mode mode, char **sp)
{
	type_ref *dp = parse_type_ref_array(mode, sp);

	while(accept(token_open_paren)){
		type_ref *r_new = type_ref_new_func(dp, parse_func_arglist());

		EAT(token_close_paren);

		dp = r_new;
	}

	return dp;
}

type_ref *parse_type_ref(void)
{
	return parse_type_ref2(0, NULL);
}

decl *parse_decl(type_ref *subtype, enum decl_mode mode)
{
	type_ref **insert;
	decl *d;
	char *spel = NULL;

	d = decl_new();

	/* don't fold typedefs until later (for __typeof) */
	d->ref = parse_type_ref2(mode, &spel);

	d->spel = spel;

	for(insert = &d->ref; *insert; insert = &(*insert)->ref);
	*insert = subtype;

	parse_add_attr(&d->attr); /* int spel __attr__ */

	if(d->spel && accept(token_assign))
		d->init = parse_initialisation();

#ifdef PARSE_DECL_VERBOSE
	fprintf(stderr, "parsed decl %s, is_func %d, at %s init=%p\n",
			d->spel, DECL_IS_FUNC(d),
			token_to_str(curtok), d->init);

	for(type_ref *dp = d->desc; dp; dp = dp->child)
		fprintf(stderr, "\tdesc %s\n", type_ref_to_str(dp->type));
#endif

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
	enum decl_storage store;
	type_ref *r = parse_type(mode & DECL_ALLOW_STORE ? &store : NULL);

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
	type_ref *r = parse_type(&store);
	decl **decls = NULL;

	if(!r)
		return NULL;

	prevent_typedef(&r->where, store);

	do{
		decl *d = parse_decl(r, DECL_SPEL_NEED);
		dynarray_add((void ***)&decls, d);
	}while(accept(token_comma));

	return decls;
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

		this_ref = parse_type(mode & DECL_MULTI_ALLOW_STORE ? &store : NULL);

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
			struct_union_enum_st *sue = type_ref_is_s_or_u(this_ref);

			if(sue && !parse_possible_decl()){
				/*
				 * struct { int i; }; - continue to next one
				 * we check the actual ->struct/enum because we don't allow "enum x;"
				 */
				enum type_qualifier qual;

				if(sue->anon)
					WARN_AT(&this_ref->where, "anonymous struct with no instances");

				/* check for storage/qual on no-instance */
				qual = type_ref_qual(this_ref);
				if(qual || store != store_default){
					WARN_AT(&this_ref->where, "ignoring %s%s%son no-instance %s",
							store != store_default ? decl_store_to_str(store) : "",
							store != store_default ? " " : "",
							type_qual_to_str(qual),
							sue_str(sue));
				}

				goto next;
			}
		}

		do{
			decl *d = parse_decl(this_ref, parse_flag);

			UCC_ASSERT(d, "null decl after parse");

			if(!d->spel){
				/*
				 * int; - fine for "int;", but "int i,;" needs to fail
				 * struct A; - fine
				 * struct { int i; }; - warn
				 */
				if(!last){
					int warn = 0;
					struct_union_enum_st *sue;

					/* allow "int : 5;" */
					if(curtok == token_colon)
						goto got_field_width;

					/* check for no-fwd and anon */
					sue = type_ref_is_s_or_u_or_e(this_ref);
					switch(sue ? sue->primitive : type_unknown){
						case type_enum:
						case type_struct:
						case type_union:
							/* if it doesn't have a ->sue, it's a forward decl */
							warn = sue && !sue->anon;
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
						if(d->ref->type != type_ref_type)
							DIE_AT(&d->where, err_nodecl);
						else
							WARN_AT(&d->where, err_nodecl);
#undef err_nodecl
					}

					d->ref = NULL;
					decl_free(d);
					goto next;
				}
				DIE_AT(&d->where, "identifier expected after decl (got %s)", token_to_str(curtok));
			}else if(DECL_IS_FUNC(d) && curtok == token_open_block){ /* this is why we can't have __attribute__ on function defs */
				/* optionally check for old func decl */
				decl **old_args = parse_decls_multi_type(0);

				if(old_args){
					/* check then replace old args */
					int n_proto_decls, n_old_args;
					int i;
					funcargs *dfuncargs = d->ref->bits.func;

					UCC_ASSERT(d->ref->type == type_ref_func, "not func");

					if(!dfuncargs->args_old_proto)
						DIE_AT(&d->where, "unexpected old-style decls - new style proto used");

					n_proto_decls = dynarray_count((void **)dfuncargs->arglist);
					n_old_args = dynarray_count((void **)old_args);

					if(n_old_args > n_proto_decls)
						DIE_AT(&d->where, "old-style function decl: too many decls");

					for(i = 0; i < n_old_args; i++)
						if(old_args[i]->init)
							DIE_AT(&old_args[i]->where, "parameter \"%s\" is initialised", old_args[i]->spel);

					for(i = 0; i < n_old_args; i++){
						int j;
						for(j = 0; j < n_proto_decls; j++){
							if(!strcmp(old_args[i]->spel, dfuncargs->arglist[j]->spel)){
								decl **replace_this;
								decl *free_this;

								/* replace the old implicit int arg */
								replace_this = &dfuncargs->arglist[j];

								free_this = *replace_this;
								*replace_this = old_args[i];

								decl_free(free_this);
								break;
							}
						}
					}

					free(old_args);
				}

				d->func_code = parse_stmt_block();
			}

got_field_width:
			dynarray_add(are_tdefs
					? (void ***)&current_scope->typedefs
					: (void ***)&decls,
					d);

			/* FIXME: check later for functions, not here - typedefs */
			if(DECL_IS_FUNC(d)){
				if(d->func_code && (mode & DECL_MULTI_ACCEPT_FUNC_CODE) == 0)
						DIE_AT(&d->where, "function code not wanted (%s)", d->spel);

				if((mode & DECL_MULTI_ACCEPT_FUNC_DECL) == 0)
					DIE_AT(&d->where, "function decl not wanted (%s)", d->spel);
			}

			if(are_tdefs){
				if(DECL_IS_FUNC(d) && d->func_code)
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
			ICE("TODO: struct inter-var padding");
			/* anon-decl with field width to pad? */
		}
	}
}
