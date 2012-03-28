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

#include "struct.h"
#include "enum.h"
#include "sym.h"

#include "cc1.h"

#include "parse.h"
#include "parse_type.h"

#include "expr.h"

type *parse_type_struct(void);
type *parse_type_enum(void);
decl_desc *parse_decl_desc_array(enum decl_mode mode, char **decl_sp, funcargs **decl_args);

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

type *parse_type_struct()
{
	type *t;
	char *spel;

	parse_type_preamble(&t, &spel, type_struct);

	if(accept(token_open_block)){
		decl **members = parse_decls_multi_type(DECL_CAN_DEFAULT | DECL_SPEL_NEED, 1);
		EAT(token_close_block);
		t->struc = struct_add(current_scope, spel, members);
	}else if(!spel){
		die_at(NULL, "expected: struct definition or name");
	}

	t->spel = spel; /* save for lookup */

	return t;
}

type *parse_type_enum()
{
	type *t;
	char *spel;

	parse_type_preamble(&t, &spel, type_enum);

	if(accept(token_open_block)){
		enum_st *en = enum_st_new();

		do{
			expr *e;
			char *sp;

			sp = token_current_spel();
			EAT(token_identifier);

			if(accept(token_assign))
				e = parse_expr_funcallarg(); /* no commas */
			else
				e = NULL;

			enum_vals_add(en, sp, e);

			if(!accept(token_comma))
				break;
		}while(curtok == token_identifier);

		EAT(token_close_block);

		t->enu = enum_add(&current_scope->enums, spel, en);
	}else if(!spel){
		die_at(NULL, "expected: enum definition or name");
	}

	t->spel = spel; /* save for lookup */

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
			if(qual != qual_none)
				die_at(NULL, "second type qualification %s", type_qual_to_str(qual));

			qual = curtok_to_type_qualifier();
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

		}else if(curtok == token_struct || curtok == token_enum){
			const int en = curtok == token_enum;
			type *t;

			EAT(curtok);
			t = en ? parse_type_enum() : parse_type_struct();

			if(signed_set || primitive_set)
				die_at(&t->where, "primitive/signed/unsigned with %s", en ? "enum" : "struct");

			t->qual  = qual;
			t->store = store;

			return t;

		}else if(curtok == token_identifier && (td = typedef_find(current_scope, token_current_spel_peek()))){
			/* typedef name */

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
			die_at(NULL, "signed/unsigned not allowed with typedef instance");


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
	/*
	 * either:
	 *
	 * [<type>] <name>( [<type> [<name>]]... )
	 * {
	 * }
	 *
	 * with optional {}
	 *
	 * or
	 *
	 * [<type>] <name>( [<name>...] )
	 *   <type> <name>, ...;
	 *   <type> <name>, ...;
	 * {
	 * }
	 *
	 * non-optional code
	 *
	 * i.e.
	 *
	 * int x(int y);
	 * int x(int y){}
	 *
	 * or
	 *
	 * int x(y)
	 *   int y;
	 * {
	 * }
	 *
	 */
	funcargs *args;
	decl *argdecl;

	args = funcargs_new();

	if(curtok == token_close_paren)
		goto empty_func;

	argdecl = parse_decl_single(DECL_CAN_DEFAULT);

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

		if(dynarray_count((void *)args->arglist) == 1
				&& args->arglist[0]->type->primitive == type_void
				&& !decl_ptr_depth(args->arglist[0])
				&& !args->arglist[0]->spel){
			/* x(void); */
			function_empty_args(args);
			args->args_void = 1; /* (void) vs () */
		}

	}else{
		int i, n_spels, n_decls;
		char **spells;
		decl **argar;

		spells = NULL;

		ICE("TODO: old functions (on %s%s%s)",
				token_to_str(curtok),
				curtok == token_identifier ? ": " : "",
				curtok == token_identifier ? token_current_spel_peek() : ""
				);

		do{
			if(curtok != token_identifier)
				EAT(token_identifier); /* error */

			dynarray_add((void ***)&spells, token_current_spel());
			EAT(token_identifier);

			if(accept(token_close_paren))
				break;
			EAT(token_comma);

		}while(1);

		/* parse decls, then check they correspond */
		argar = PARSE_DECLS();

		n_decls = dynarray_count((void *)argar);
		n_spels = dynarray_count((void *)spells);

		if(n_decls > n_spels)
			die_at(argar ? &argar[0]->where : &args->where, "old-style function decl: mismatching argument counts");

		for(i = 0; i < n_spels; i++){
			int j, found;

			found = 0;
			for(j = 0; j < n_decls; j++)
				if(!strcmp(spells[i], argar[j]->spel)){
					if(argar[j]->init)
						die_at(&argar[j]->where, "parameter \"%s\" is initialised", argar[j]->spel);

					found = 1;
					break;
				}

			if(!found){
				/*
					* void f(x){ ... }
					* - x is implicitly int
					*/
				decl *d = decl_new();
				d->type->primitive = type_int;
				d->spel = spells[i];
				spells[i] = NULL; /* prevent free */
				dynarray_add((void ***)&argar, d);
			}
		}

		/* no need to check the other way around, since the counts are equal */
		if(spells)
			dynarray_free((void ***)&spells, free);

		args->arglist = argar;

		if(curtok != token_open_block)
			die_at(&args->where, "no code for old-style function");
		//args->code = parse_code();
	}

empty_func:

	return args;
}

decl_desc *parse_decl_desc_func(void)
{
	if(accept(token_open_paren)){
		/*
		 * e.g.:
		 * int (*x)(
		 *         ^
		 */

		if(ret->fptrargs)
			goto func_ret_func;

		ret->fptrargs = parse_func_arglist();
		EAT(token_close_paren);

		if(accept(token_open_paren))
func_ret_func:
			die_at(&ret->where, "can't have function returning function");
	}
}

decl_desc *parse_decl_desc_rec(enum decl_mode mode, char **decl_sp, funcargs **decl_args)
{
	decl_desc *ret = NULL;

	if(accept(token_open_paren)){
		ret = parse_decl_desc_array(mode, decl_sp, decl_args);
		EAT(token_close_paren);

	}else if(accept(token_multiply)){
		ret = decl_desc_ptr_new();

		if(accept(token_const))
			ret->is_const = 1;

		ret->child = parse_decl_desc_func();

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
	return ret;
}

decl_desc *parse_decl_desc_array(enum decl_mode mode, char **decl_sp, funcargs **decl_args)
{
	decl_desc *dp = parse_decl_desc_rec(mode, decl_sp, decl_args);

	while(accept(token_open_square)){
		decl_desc *dp_new;
		expr *size;

		if(accept(token_close_square)){
			/* take size as zero */
			size = expr_new_val(0);
		}else{
			/* fold.c checks for const-ness */
			size = parse_expr();
			EAT(token_close_square);
		}

		dp_new = decl_desc_array_new();

		/* is this right? t'other way around? append to leaf? */
		dp_new->child = dp;
		dp_new->array_size = size;

		dp = dp_new;
	}

	return dp;
}

decl_attr *parse_attr(void)
{
	decl_attr *attr = NULL;

	for(;;){
		char *ident;

		if(curtok != token_identifier)
			die_at(NULL, "identifier expected for attribute");

		ident = token_current_spel();

		if(!strcmp(ident, "__format__")){
			/* printf-like */
			EAT(token_open_paren);
			ICE("TODO");
			/*
         #define __printflike(fmtarg, firstvararg) \
          __attribute__((__format__ (__printf__, fmtarg, firstvararg)))
			*/

		}else{
			warn_at(&attr->where, "ignoring unrecognised attribute \"%s\"\n", ident);
			free(ident);
		}
	}
}

decl *parse_decl(type *t, enum decl_mode mode)
{
	char *spel = NULL;
	funcargs *args = NULL;
	decl_desc *dp;
	decl *d;

	dp = parse_decl_desc_array(mode, &spel, &args);

	/* don't fold typedefs until later (for __typeof) */
	d = decl_new();
	d->desc = dp;
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

	if(spel && accept(token_assign))
		d->init = parse_expr_funcallarg(); /* int x = 5, j; - don't grab the comma expr */
	else if(d->funcargs && curtok == token_open_block)
		d->func_code = parse_code();

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
		else if((t->struc || t->enu) && !parse_possible_decl())
			goto next; /* struct { int i; }; - continue to next one */

		do{
			decl *d = parse_decl(t, parse_flag);

			if(!d)
				break;

			if(!d->spel){
				/*
				 * int; - fine for "int;", but "int i,;" needs to fail
				 * struct A; - fine
				 */
				decl_free_notype(d);
				if(!last){
					if(t->primitive == type_struct ? !t->struc->anon : 1)
						warn_at(&d->where, "declaration doesn't declare anything");
					goto next;
				}
				die_at(&d->where, "identifier expected after decl");
			}

			dynarray_add(are_tdefs
					? (void ***)&current_scope->typedefs
					:  (void ***)&decls,
					d);

			if(are_tdefs)
				if(d->funcargs)
					die_at(&d->where, "can't have a typedef function");

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
