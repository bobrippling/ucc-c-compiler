#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "tree.h"
#include "typedef.h"

#include "tokenise.h"
#include "tokconv.h"

#include "struct.h"
#include "enum.h"
#include "sym.h"

#include "cc1.h"

#include "parse.h"
#include "parse_type.h"

type *parse_type_struct(void);
type *parse_type_enum(void);
decl_ptr *parse_decl_ptr(enum decl_mode mode);

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
		decl **members = parse_decls(DECL_CAN_DEFAULT | DECL_SPEL_NEED, 1);
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
	enum type_spec spec;
	type *t;
	decl *td;
	int flag;

	spec = 0;
	t = NULL;

	/* read "const", "unsigned", ... and "int"/"long" ... in any order */
	while(td = NULL,
			((flag = curtok_is_type_specifier()) ||
			curtok_is_type() ||
			curtok == token_struct ||
			curtok == token_enum)){

		if(accept(token_struct)){
			return parse_type_struct();
		}else if(accept(token_enum)){
			return parse_type_enum();
		}else if(flag){
			const enum type_spec this = curtok_to_type_specifier();

			UCC_ASSERT(this != spec_none, "spec none where spec expected");

			/* we can't check in fold, since 1 & 1 & 1 is still just 1 */
			if(this & spec)
				die_at(NULL, "duplicate type specifier \"%s\"", spec_to_str(spec));

			spec |= this;
			EAT(curtok);
		}else if(td){
			/* typedef name */

			if(t){
				/* "int x" - we are at x, which is also a typedef somewhere */
				/*cc1_warn_at(NULL, 0, WARN_IDENT_TYPEDEF, "identifier is a typedef name");*/
				break;
			}

			t = type_new();
			t->primitive = type_typedef;
			t->tdef = td;

			EAT(token_identifier);

		}else{
			/* curtok_is_type */
			if(t){
				die_at(NULL, "second type name unexpected");
			}else{
				t = type_new();
				t->primitive = curtok_to_type_primitive();
				UCC_ASSERT(t->primitive != type_unknown, "unknown type where type expected");
				EAT(curtok);
			}
		}
	}

	if(!t && spec)
		/* unsigned x; */
		INT_TYPE(t);

	if(t)
		t->spec = spec;

	return t;
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

	if(curtok == token_identifier)
		goto old_func;

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

		if(dynarray_count((void *)args->arglist) == 1 &&
				                      args->arglist[0]->type->primitive == type_void &&
				                     !args->arglist[0]->decl_ptr->child && /* manual checks, since decl_*() assert */
				                     !args->arglist[0]->decl_ptr->spel){
			/* x(void); */
			function_empty_args(args);
			args->args_void = 1; /* (void) vs () */
		}

	}else{
		int i, n_spels, n_decls;
		char **spells;
		decl **argar;

old_func:
		spells = NULL;

		ICE("TODO: old functions (on %s)", token_to_str(curtok));

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
				if(!strcmp(spells[i], decl_spel(argar[j]))){
					if(argar[j]->init)
						die_at(&argar[j]->where, "parameter \"%s\" is initialised", decl_spel(argar[j]));

					found = 1;
					break;
				}

			if(!found){
				/*
					* void f(x){ ... }
					* - x is implicitly int
					*/
				decl *d = decl_new_with_ptr();
				d->type->primitive = type_int;
				decl_set_spel(d, spells[i]);
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

decl_ptr *parse_decl_ptr_nofunc(enum decl_mode mode)
{
	if(accept(token_open_paren)){
		decl_ptr *ret = parse_decl_ptr(mode);
		EAT(token_close_paren);
		return ret;

	}else if(accept(token_multiply)){
		decl_ptr *ret = decl_ptr_new();

		if(accept(token_const))
			ret->is_const = 1;

		ret->child = parse_decl_ptr(mode); /* check if we have anything else */
		if(!ret->child)
			ret->child = decl_ptr_new(); /* if not, we need to ensure we mark that it's a pointer */
		return ret;

	}else if(curtok == token_identifier){
		decl_ptr *ret = decl_ptr_new();

		if(mode & DECL_SPEL_NO)
			die_at(&ret->where, "identifier unexpected");

		ret->spel = token_current_spel();
		EAT(token_identifier);

		return ret;
	}else if(mode & DECL_SPEL_NEED){
		die_at(NULL, "need identifier for decl");
	}

	return NULL;
}

decl_ptr *parse_decl_ptr(enum decl_mode mode)
{
	decl_ptr *dp;

	dp = parse_decl_ptr_nofunc(mode);

	if(accept(token_open_paren)){
		/*
			* e.g.:
			* int x(
			* int (*x)(
			* int (((x))(
			*/
		if(dp->func)
			goto func_ret_func;

		dp->func = parse_func_arglist();
		EAT(token_close_paren);
	}else{
		while(accept(token_open_square)){
			expr *size;
			int fin;
			fin = 0;

			if(curtok != token_close_square)
				size = parse_expr(); /* fold.c checks for const-ness */
			else
				fin = 1;

			//d->ptr_depth++; /* here is where we need to recurse? */
			ICE("recursive array decl parsing");
			EAT(token_close_square);

			if(fin)
				break;

			dynarray_add((void ***)&dp->array_sizes, size);
		}
	}

	if(accept(token_open_paren))
func_ret_func:
		die_at(&dp->where, "can't have function returning function");

	return dp;
}

decl *parse_decl(type *t, enum decl_mode mode)
{
	decl *d = decl_new();
	decl_ptr *dp = parse_decl_ptr(mode);

	if(!dp)
		dp = decl_ptr_new(); /* (int)x - no ->dp for "int", add one */

	d->type = t;
	d->decl_ptr = dp;
	dp->is_const |= (t->spec & spec_const); /* we never check t->spec, just ->is_const */

	if(accept(token_assign))
		d->init = parse_expr_funcallarg(); /* int x = 5, j; - don't grab the comma expr */
	else if(curtok == token_open_block)
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

	if(t->spec & spec_typedef)
		die_at(&t->where, "typedef unexpected");

	return parse_decl(t, mode);
}

decl **parse_decls(const int can_default, const int accept_field_width)
{
	const enum decl_mode parse_flag = DECL_SPEL_NEED | (can_default ? DECL_CAN_DEFAULT : 0);
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

		if(t->spec & spec_typedef)
			are_tdefs = 1;
		else if((t->struc || t->enu) && !parse_possible_decl())
			goto next; /* struct { int i; }; - continue to next one */

		do{
			decl *d = parse_decl(t, parse_flag);

			if(!d)
				break;

			dynarray_add(are_tdefs
					? (void ***)&current_scope->typedefs
					:  (void ***)&decls,
					d);

			if(are_tdefs)
				if(decl_has_func_code(d))
					die_at(&d->where, "can't have a typedef function");

			if(decl_has_func_code(d)){
				if(curtok == token_eof)
					return decls;
				continue;
			}else if(accept_field_width && accept(token_colon)){
				/* normal decl, check field spec */
				d->field_width = currentval.val;
				if(d->field_width <= 0)
					die_at(&d->where, "field width must be positive");
				EAT(token_integer);
			}

			last = d;
		}while(accept(token_comma));

		if(last && !decl_has_func_code(last)){
next:
			EAT(token_semicolon);
		}
	}
}
