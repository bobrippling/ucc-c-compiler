#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "data_structs.h"
#include "macros.h"
#include "sym.h"
#include "../util/platform.h"
#include "sue.h"
#include "decl.h"

const where *eof_where = NULL;

intval *intval_new(long v)
{
	intval *iv = umalloc(sizeof *iv);
	iv->val = v;
	return iv;
}


void where_new(struct where *w)
{
	extern int buffereof;

	if(buffereof){
		if(eof_where)
			memcpy(w, eof_where, sizeof *w);
		else
			ICE("where_new() after buffer eof");

	}else{
		extern int current_line, current_chr;
		extern const char *current_fname, *current_line_str;
		extern int current_fname_used, current_line_str_used;

		w->line  = current_line;
		w->chr   = current_chr;
		w->fname = current_fname;
		w->line_str = current_line_str;

		UCC_ASSERT(current_fname, "no current fname");
		UCC_ASSERT(current_line_str, "no current line");

		current_fname_used = 1;
		current_line_str_used = 1;
	}
}

type *type_new()
{
	type *t = umalloc(sizeof *t);
	where_new(&t->where);
	t->is_signed = 1;
	t->primitive = type_unknown;
	return t;
}

type *type_copy(type *t)
{
	type *ret = umalloc(sizeof *ret);
	memcpy(ret, t, sizeof *ret);
	return ret;
}

void funcargs_free(funcargs *args, int free_decls)
{
	if(free_decls){
		int i;
		for(i = 0; args->arglist[i]; i++)
			decl_free(args->arglist[i]);
	}
	free(args);
}

int type_primitive_size(enum type_primitive tp)
{
	switch(tp){
		case type_char:
		case type__Bool:
		case type_void:
			return 1;

		case type_short:
			return 2;

		case type_int:
		case type_float:
			return 4;

		case type_long:
		case type_double:
			return 8; /* FIXME: 4 on 32-bit */

		case type_llong:
			ICW("TODO: long long");
			return 16;

		case type_ldouble:
			/* 80-bit float */
			ICW("TODO: long double");
			return 10; /* FIXME: 32-bit? */

		case type_union:
		case type_struct:
		case type_enum:
			ICE("sue size");

		case type_unknown:
			break;
	}

	ICE("type %s in type_size()", type_primitive_to_str(tp));
	return -1;
}

int type_size(const type *t)
{
	if(t->typeof)
		return decl_size(t->typeof->decl);

	if(t->sue)
		return sue_size(t->sue);

	return type_primitive_size(t->primitive);
}

int type_equal(const type *a, const type *b, int strict)
{
	if(strict && b->qual != a->qual)
		return 0;

	if(a->sue != b->sue)
		return 0;

	return strict ? a->primitive == b->primitive : 1;
}

void function_empty_args(funcargs *func)
{
	if(func->arglist){
		UCC_ASSERT(!func->arglist[1], "empty_args called when it shouldn't be");

		decl_free(func->arglist[0]);
		free(func->arglist);
		func->arglist = NULL;
	}
	func->args_void = 0;
}

funcargs *funcargs_new()
{
	funcargs *r = umalloc(sizeof *funcargs_new());
	where_new(&r->where);
	return r;
}

const char *op_to_str(const enum op_type o)
{
	switch(o){
		CASE_STR_PREFIX(op, multiply);
		CASE_STR_PREFIX(op, divide);
		CASE_STR_PREFIX(op, plus);
		CASE_STR_PREFIX(op, minus);
		CASE_STR_PREFIX(op, modulus);
		CASE_STR_PREFIX(op, eq);
		CASE_STR_PREFIX(op, ne);
		CASE_STR_PREFIX(op, le);
		CASE_STR_PREFIX(op, lt);
		CASE_STR_PREFIX(op, ge);
		CASE_STR_PREFIX(op, gt);
		CASE_STR_PREFIX(op, or);
		CASE_STR_PREFIX(op, xor);
		CASE_STR_PREFIX(op, and);
		CASE_STR_PREFIX(op, orsc);
		CASE_STR_PREFIX(op, andsc);
		CASE_STR_PREFIX(op, not);
		CASE_STR_PREFIX(op, bnot);
		CASE_STR_PREFIX(op, shiftl);
		CASE_STR_PREFIX(op, shiftr);
		CASE_STR_PREFIX(op, unknown);
	}
	return NULL;
}

const char *type_primitive_to_str(const enum type_primitive p)
{
	switch(p){
		CASE_STR_PREFIX(type, void);
		CASE_STR_PREFIX(type, char);
		CASE_STR_PREFIX(type, short);
		CASE_STR_PREFIX(type, int);
		CASE_STR_PREFIX(type, long);
		CASE_STR_PREFIX(type, float);
		CASE_STR_PREFIX(type, double);
		CASE_STR_PREFIX(type, _Bool);

		case type_llong:   return "long long";
		case type_ldouble: return "long double";

		CASE_STR_PREFIX(type, struct);
		CASE_STR_PREFIX(type, union);
		CASE_STR_PREFIX(type, enum);

		CASE_STR_PREFIX(type, unknown);
	}
	return NULL;
}

const char *type_store_to_str(const enum type_storage s)
{
	switch(s){
		CASE_STR_PREFIX(store, default);
		CASE_STR_PREFIX(store, auto);
		CASE_STR_PREFIX(store, static);
		CASE_STR_PREFIX(store, extern);
		CASE_STR_PREFIX(store, register);
		CASE_STR_PREFIX(store, typedef);
	}
	return NULL;
}

const char *type_qual_to_str(const enum type_qualifier qual)
{
	static char buf[32];
	/* trailing space is purposeful */
	snprintf(buf, sizeof buf, "%s%s%s",
		qual & qual_const    ? "const "    : "",
		qual & qual_volatile ? "volatile " : "",
		qual & qual_restrict ? "restrict " : "");
	return buf;
}

int op_can_compound(enum op_type o)
{
	switch(o){
		case op_plus:
		case op_minus:
		case op_multiply:
		case op_divide:
		case op_modulus:
		case op_not:
		case op_bnot:
		case op_and:
		case op_or:
		case op_xor:
		case op_shiftl:
		case op_shiftr:
			return 1;
		default:
			break;
	}
	return 0;
}

int op_is_relational(enum op_type o)
{
	switch(o){
		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
		case op_andsc:
		case op_orsc:
			return 1;
		default:
			break;
	}
	return 0;
}

const char *type_to_str(const type *t)
{
#define BUF_SIZE (sizeof(buf) - (bufp - buf))
	static char buf[TYPE_STATIC_BUFSIZ];
	char *bufp = buf;

	if(t->typeof)     bufp += snprintf(bufp, BUF_SIZE, "typedef ");

	{
		const char *tmp = type_qual_to_str(t->qual);
		bufp += snprintf(bufp, BUF_SIZE, "%s", tmp);
	}

	if(t->store)      bufp += snprintf(bufp, BUF_SIZE, "%s ", type_store_to_str(t->store));
	if(!t->is_signed) bufp += snprintf(bufp, BUF_SIZE, "unsigned ");
	if( t->is_inline) bufp += snprintf(bufp, BUF_SIZE, "inline ");

	if(t->sue){
		snprintf(bufp, BUF_SIZE, "%s%s %s",
				sue_incomplete(t->sue) ? "incomplete-" : "",
				sue_str(t->sue),
				t->sue->spel);

	}else{
		switch(t->primitive){
#define SAPPEND(s) snprintf(bufp, BUF_SIZE, "%s", s); break
#define APPEND(t) case type_ ## t: SAPPEND(#t)
			APPEND(void);
			APPEND(_Bool);
			APPEND(char);
			APPEND(short);
			APPEND(int);
			APPEND(long);
			APPEND(float);
			APPEND(double);

			case type_llong:   SAPPEND("long long");
			case type_ldouble: SAPPEND("long double");

			case type_unknown:
				ICE("unknown type primitive (%s)", where_str(&t->where));
			case type_enum:
				ICE("enum without ->enu");
			case type_struct:
			case type_union:
				ICE("struct/union without ->struct_union");
#undef APPEND
		}
	}

	return buf;
}
