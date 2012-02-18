#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../util/util.h"
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

type  *parse_type_struct(void);
type *parse_type_enum(void);

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
	// TODO: recursive descent
	enum type_spec spec;
	type *t;
	decl *td;
	int flag;

	spec = 0;
	t = NULL;

	/* read "const", "unsigned", ... and "int"/"long" ... in any order */
	while(td = NULL,
			(flag = curtok_is_type_specifier()) ||
			curtok_is_type() ||
			curtok == token_struct ||
			curtok == token_enum){

		if(accept(token_struct)){
			return parse_type_struct();
		}else if(accept(token_enum)){
			return parse_type_enum();
		}else if(flag){
			const enum type_spec this = curtok_to_type_specifier();

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

decl *parse_decl(type *t, enum decl_mode mode)
{
	decl *d = decl_new();

	d->type = t;

	while(accept(token_multiply))
		/* FIXME: int *const x; */
		d->ptr_depth++;

	if(t->tdef){
		if(t->tdef->arraysizes)
			ICE("todo: typedef with arraysizes");
		d->ptr_depth += t->tdef->ptr_depth;
	}

	if(curtok == token_identifier){
		if(mode & DECL_SPEL_NO)
			die_at(&d->where, "identifier unexpected");

		d->spel = token_current_spel();
		EAT(token_identifier);
	}else if(mode & DECL_SPEL_NEED){
		die_at(&d->where, "need identifier for decl");
	}

	if(accept(token_open_paren))
		d->func = parse_function();

	while(accept(token_open_square)){
		expr *size;
		int fin;
		fin = 0;

		if(curtok != token_close_square)
			size = parse_expr(); /* fold.c checks for const-ness */
		else
			fin = 1;

		d->ptr_depth++;
		EAT(token_close_square);

		if(fin)
			break;

		dynarray_add((void ***)&d->arraysizes, size);
	}

	if(accept(token_assign))
		d->init = parse_expr_funcallarg(); /* int x = 5, j; - don't grab the comma expr */

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
				if(d->func)
					die_at(&d->where, "can't have a typedef function");

			if(d->func){
				if(d->func->code){
					if(curtok == token_eof)
						return decls;
					continue;
				}
			}else if(accept_field_width && accept(token_colon)){
				/* normal decl, check field spec */
				d->field_width = currentval.val;
				EAT(token_integer);
			}

			last = d;
		}while(accept(token_comma));

		if(last && !last->func){
next:
			EAT(token_semicolon);
		}
	}
}
