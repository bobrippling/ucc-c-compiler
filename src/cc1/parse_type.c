#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "typedef.h"

#include "tokenise.h"
#include "tokconv.h"

#include "sue.h"
#include "sym.h"

#include "cc1.h"

#include "parse.h"
#include "parse_type.h"

#include "expr.h"

type *parse_type_struct(void);
decl_ptr *parse_decl_ptr_array(enum decl_mode mode, char **decl_sp, funcargs **decl_args);
decl  *parse_decl(type *t, enum decl_mode mode);

#define INT_TYPE(t) do{ t = type_new(); t->primitive = type_int; }while(0)

void parse_type_preamble(type **tp, char **psp, enum type_primitive primitive)
{
	char *spel;
	type *t;

	spel = NULL;
	t = type_new();
	t->primitive = primitive;

	if(curtok == token_identifier){
		spel = token_current_spel();
		EAT(token_identifier);
	}

	*psp = spel;
	*tp = t;
}

type *parse_type_struct_union(enum type_primitive prim)
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
					e = parse_expr_funcallarg(); /* no commas */
				else
					e = NULL;

				enum_vals_add(&members, sp, e);

				if(!accept(token_comma))
					break;
			}while(curtok == token_identifier);

			EAT(token_close_block);
		}else{
			members = (sue_member **)parse_decls_multi_type(DECL_CAN_DEFAULT | DECL_SPEL_NEED, 1);
			EAT(token_close_block);
		}

	}else if(!spel){
		die_at(NULL, "expected: struct definition or name");
	}

	t->sue = sue_add(current_scope, spel, members, prim);

	return t;
}

type *parse_type()
{
	expr *tdef_typeof = NULL;
	enum type_qualifier qual = qual_none;
	enum type_storage   store = store_auto;
	enum type_primitive primitive = type_int;
	int is_signed = 1;
	int store_set = 0, primitive_set = 0, signed_set = 0;

	if(accept(token_typeof)){
		type *t = type_new();
		t->typeof = parse_expr_sizeof_typeof();
		t->typeof->expr_is_typeof = 1;
		return t;
	}

	for(;;){
		decl *td;

		if(curtok_is_type_qual()){
			qual |= curtok_to_type_qualifier();
			EAT(curtok);

		}else if(curtok_is_type_store()){
			store = curtok_to_type_storage();

			if(store_set)
				die_at(NULL, "second type store %s", type_store_to_str(store));

			store_set = 1;
			EAT(curtok);

		}else if(curtok_is_type_primitive()){
			primitive = curtok_to_type_primitive();

			if(primitive_set)
				die_at(NULL, "second type primitive %s", type_primitive_to_str(primitive));

			primitive_set = 1;
			EAT(curtok);

		}else if(curtok == token_signed || curtok == token_unsigned){
			is_signed = curtok == token_signed;

			if(signed_set)
				die_at(NULL, "unwanted second \"%ssigned\"", is_signed ? "" : "un");

			signed_set = 1;
			EAT(curtok);

		}else if(curtok == token_struct || curtok == token_union || curtok == token_enum){
			const enum token tok = curtok;
			const char *str;
			type *t;

			EAT(curtok);

			switch(tok){
#define CASE(a)                                    \
				case token_ ## a:                          \
					t = parse_type_struct_union(type_ ## a); \
					str = #a;                                \
					break

				CASE(enum);
				CASE(struct);
				CASE(union);

				default:
					ICE("wat");
			}

			if(signed_set || primitive_set)
				die_at(&t->where, "primitive/signed/unsigned with %s", str);

			t->qual  = qual;
			t->store = store;

			return t;

		}else if(curtok == token_identifier && (td = typedef_find(current_scope, token_current_spel_peek()))){
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
				cc1_warn_at(NULL, 0, WARN_IDENT_TYPEDEF, "identifier is a typedef name");
				break;
			}

			tdef_typeof = expr_new_sizeof_decl(td);
			primitive_set = 1;

			EAT(token_identifier);
			break;
		}else{
			break;
		}
	}

	if(qual != qual_none || store_set || primitive_set || signed_set || tdef_typeof){
		type *t = type_new();

		/* signed size_t x; */
		if(tdef_typeof && signed_set)
			die_at(NULL, "signed/unsigned not allowed with typedef instance (%s)", tdef_typeof->decl->spel);


		if(!primitive_set)
			INT_TYPE(t); /* unsigned x; */
		else
			t->primitive = primitive;

		t->typeof = tdef_typeof;
		t->is_signed = is_signed;
		t->qual  = qual;
		t->store = store;

		return t;
	}else{
		return NULL;
	}
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
		for(;;){
			dynarray_add((void ***)&args->arglist, argdecl);

			if(curtok == token_close_paren)
				break;

			EAT(token_comma);

			if(accept(token_elipsis)){
				args->variadic = 1;
				break;
			}

			/* continue loop */
			/* actually, we don't need a type here, default to int, i think */
			argdecl = parse_decl_single(DECL_CAN_DEFAULT);
		}

		if(dynarray_count((void *)args->arglist) == 1 &&
				                      args->arglist[0]->type->primitive == type_void &&
				                     !args->arglist[0]->decl_ptr && /* manual checks, since decl_*() assert */
				                     !args->arglist[0]->spel){
			/* x(void); */
			function_empty_args(args);
			args->args_void = 1; /* (void) vs () */
		}

	}else{
		do{
			decl *d = decl_new();

			if(curtok != token_identifier)
				EAT(token_identifier); /* error */

			d->type->primitive = type_int;
			d->spel = token_current_spel();
			dynarray_add((void ***)&args->arglist, d);

			EAT(token_identifier);

			if(curtok == token_close_paren)
				break;
			EAT(token_comma);
		}while(1);
		args->args_old_proto = 1;
	}

empty_func:

	return args;
}

decl_ptr *parse_decl_ptr_rec(enum decl_mode mode, char **decl_sp, funcargs **decl_args)
{
	decl_ptr *ret = NULL;

	if(accept(token_open_paren)){
		ret = parse_decl_ptr_array(mode, decl_sp, decl_args);
		EAT(token_close_paren);

	}else if(accept(token_multiply)){
		ret = decl_ptr_new();

		if(accept(token_const))
			ret->is_const = 1;

		ret->child = parse_decl_ptr_array(mode, decl_sp, decl_args); /* check if we have anything else */

	}else{
		if(curtok == token_identifier){
			if(mode & DECL_SPEL_NO)
				die_at(NULL, "identifier unexpected");

			if(*decl_sp)
				die_at(NULL, "already got identifier for decl");
			*decl_sp = token_current_spel();
			EAT(token_identifier);

		}else if(mode & DECL_SPEL_NEED){
			die_at(NULL, "need identifier for decl");
		}

		/*
		 * here is the end of the line, from now on,
		 * we are coming out of the recursion
		 * so we check for the initial decl func parms
		 */
		if(accept(token_open_paren)){
			*decl_args = parse_func_arglist();
			EAT(token_close_paren);
		}
	}

	if(ret && accept(token_open_paren)){
		/*
			* e.g.:
			* int (*x)(
			*/
		if(ret->fptrargs)
			goto func_ret_func;

		ret->fptrargs = parse_func_arglist();
		EAT(token_close_paren);

		if(accept(token_open_paren))
func_ret_func:
			die_at(&ret->where, "can't have function returning function");
	}

	return ret;
}

decl_ptr *parse_decl_ptr_array(enum decl_mode mode, char **decl_sp, funcargs **decl_args)
{
	decl_ptr *dp = parse_decl_ptr_rec(mode, decl_sp, decl_args);

	while(accept(token_open_square)){
		decl_ptr *dp_new;
		expr *size;

		if(accept(token_close_square)){
			/* take size as zero */
			size = expr_new_val(0);
		}else{
			/* fold.c checks for const-ness */
			size = parse_expr();
			EAT(token_close_square);
		}

		dp_new = decl_ptr_new();

		/* is this right? t'other way around? append to leaf? */
		dp_new->child = dp;
		dp_new->array_size = size;

		dp = dp_new;
	}

	return dp;
}

#include "parse_attr.c"

decl *parse_decl(type *t, enum decl_mode mode)
{
	char *spel = NULL;
	funcargs *args = NULL;
	decl_ptr *dp;
	decl *d;

	dp = parse_decl_ptr_array(mode, &spel, &args);

	/* don't fold typedefs until later (for __typeof) */
	d = decl_new();
	d->decl_ptr = dp;
	d->type = type_copy(t);

	d->spel     = spel;
	d->funcargs = args;

	if(accept(token_attribute)){
		EAT(token_open_paren);
		EAT(token_open_paren);

		d->attr = parse_attr();

		EAT(token_close_paren);
		EAT(token_close_paren);
	}

	if(spel && accept(token_assign)){
		d->init = parse_expr_funcallarg(); /* int x = 5, j; - don't grab the comma expr */
	}else if(d->funcargs && curtok != token_semicolon){
		/* optionally check for old func decl */
		decl **old_args = PARSE_DECLS();

		if(old_args){
			/* check then replace old args */
			int n_proto_decls, n_old_args;
			int i;

			if(!d->funcargs->args_old_proto)
				die_at(&d->where, "unexpected old-style decls - new style proto used");

			n_proto_decls = dynarray_count((void **)d->funcargs->arglist);
			n_old_args = dynarray_count((void **)old_args);

			if(n_old_args > n_proto_decls)
				die_at(&d->where, "old-style function decl: too many decls");

			for(i = 0; i < n_old_args; i++)
				if(old_args[i]->init)
					die_at(&old_args[i]->where, "parameter \"%s\" is initialised", old_args[i]->spel);

			for(i = 0; i < n_old_args; i++){
				int j;
				for(j = 0; j < n_proto_decls; j++){
					if(!strcmp(old_args[i]->spel, d->funcargs->arglist[j]->spel)){
						decl **replace_this;
						decl *free_this;

						/* replace the old implicit int arg */
						replace_this = &d->funcargs->arglist[j];

						free_this = *replace_this;
						*replace_this = old_args[i];

						decl_free(free_this);
						break;
					}
				}
			}

			free(old_args);

			if(curtok != token_open_block)
				die_at(&args->where, "no code for old-style function");
		}

		d->func_code = parse_code();
	}

	return d;
}

decl *parse_decl_single(enum decl_mode mode)
{
	type *t = parse_type();

	if(!t){
		if(mode & DECL_CAN_DEFAULT){
			INT_TYPE(t);
			cc1_warn_at(&t->where, 0, WARN_IMPLICIT_INT, "defaulting type to int");
		}else{
			return NULL;
		}
	}

	if(t->store == store_typedef)
		die_at(&t->where, "typedef unexpected");

	return parse_decl(t, mode);
}

decl **parse_decls_one_type()
{
	type *t = parse_type();
	decl **decls = NULL;

	if(!t)
		return NULL;

	if(t->store == store_typedef)
		die_at(&t->where, "typedef unexpected");

	do{
		decl *d = parse_decl(t, DECL_SPEL_NEED);

		if(!d)
			die_at(&t->where, "decl expected");

		dynarray_add((void ***)&decls, d);
	}while(accept(token_comma));

	return decls;
}

decl **parse_decls_multi_type(const int can_default, const int accept_field_width)
{
	const enum decl_mode parse_flag = can_default ? DECL_CAN_DEFAULT : 0;
	decl **decls = NULL;
	decl *last;
	int are_tdefs;

	/* read a type, then *spels separated by commas, then a semi colon, then repeat */
	for(;;){
		type *t;

		last = NULL;
		are_tdefs = 0;

		t = parse_type();

		if(!t){
			/* can_default makes sure we don't parse { int *p; *p = 5; } the latter as a decl */
			if(parse_possible_decl() && can_default){
				INT_TYPE(t);
				cc1_warn_at(&t->where, 0, WARN_IMPLICIT_INT, "defaulting type to int");
			}else{
				return decls;
			}
		}

		if(t->store == store_typedef)
			are_tdefs = 1;
		else if(t->sue && !parse_possible_decl())
			goto next; /*
									* struct { int i; }; - continue to next one
									* we check the actual ->struct/enum because we don't allow "enum x;"
									*/

		do{
			decl *d = parse_decl(t, parse_flag);

			if(!d)
				break;

			if(!d->spel){
				/*
				 * int; - fine for "int;", but "int i,;" needs to fail
				 * struct A; - fine
				 */
				if(!last){
					int warn = 0;

					switch(t->primitive){
						case type_struct:
						case type_union:
						case type_enum:
							warn = t->sue && !t->sue->anon;
							break;
						default:
							warn = 1;
					}

					if(warn)
						warn_at(&d->where, "declaration doesn't declare anything");

					decl_free_notype(d);
					goto next;
				}
				die_at(&d->where, "identifier expected after decl");
			}

			dynarray_add(are_tdefs
					? (void ***)&current_scope->typedefs
					:  (void ***)&decls,
					d);

			if(are_tdefs){
				if(d->funcargs)
					die_at(&d->where, "can't have a typedef function");
				else if(d->init)
					die_at(&d->where, "can't init a typedef");
			}

			if(accept_field_width && accept(token_colon)){
				/* normal decl, check field spec */
				d->field_width = currentval.val;
				if(d->field_width <= 0)
					die_at(&d->where, "field width must be positive");
				EAT(token_integer);
			}

			last = d;
		}while(accept(token_comma));

		if(last && !last->func_code){
next:
			EAT(token_semicolon);
		}
	}
}
