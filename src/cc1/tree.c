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

where *eof_where = NULL;

void where_new(struct where *w)
{
	extern int current_line, current_chr;
	extern const char *current_fname;
	extern int buffereof;

	if(buffereof){
		if(eof_where)
			memcpy(w, eof_where, sizeof *w);
		else
			memset(w, 0, sizeof *w); /*ICE("where_new() after buffer eof");*/
	}else{
		extern int current_fname_used;

		w->line  = current_line;
		w->chr   = current_chr;
		w->fname = current_fname;

		UCC_ASSERT(current_fname, "no current fname");

		current_fname_used = 1;
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

int type_size(const type *t)
{
	if(t->typeof)
		return decl_size(t->typeof->decl);

	switch(t->primitive){
		case type_char:
		case type_void:
			return 1;

		case type_enum:
		case type_int:
			/* FIXME: 4 for int */
			return platform_word_size();

		case type_union:
		case type_struct:
			return struct_union_size(t->sue);

		case type_unknown:
			break;
	}

	ICE("type %s in type_size()", type_to_str(t));
	return -1;
}

int type_equal(const type *a, const type *b, int strict)
{
	if(strict && (b->qual & qual_const) && (a->qual & qual_const) == 0)
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
		CASE_STR_PREFIX(op, deref);
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
		CASE_STR_PREFIX(op, struct_ptr);
		CASE_STR_PREFIX(op, struct_dot);
		CASE_STR_PREFIX(op, unknown);
	}
	return NULL;
}

const char *type_primitive_to_str(const enum type_primitive p)
{
	switch(p){
		CASE_STR_PREFIX(type, int);
		CASE_STR_PREFIX(type, char);
		CASE_STR_PREFIX(type, void);

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

int op_is_cmp(enum op_type o)
{
	switch(o){
		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
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

	if(t->sue){
		snprintf(bufp, BUF_SIZE, "%s%s %s",
				sue_incomplete(t->sue) ? "incomplete-" : "",
				sue_str(t->sue),
				t->sue->spel);

	}else{
		switch(t->primitive){
#define APPEND(t) case type_ ## t: snprintf(bufp, BUF_SIZE, "%s", #t); break
			APPEND(int);
			APPEND(char);
			APPEND(void);
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
