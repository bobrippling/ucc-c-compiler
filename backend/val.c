#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include <execinfo.h>
_Noreturn void exit(int);
#undef assert
#define assert(x) ({ if(!(x)) exit(2); })

#include "mem.h"
#include "dynmap.h"

#include "val.h"
#include "val_internal.h"

#include "isn.h"
#include "op.h"

struct val
{
	enum val_type
	{
		INT,
		INT_PTR,
		NAME,
		ALLOCA
	} type;

	union
	{
		int i;
		char *name;
		int alloca_cnt;
	} u;
};

enum val_to
{
	LITERAL  = 1 << 0,
	NAMED = 1 << 1,
	ADDRESSABLE = 1 << 2,
};

static bool val_in(val *v, enum val_to to)
{
	switch(v->type){
		case INT:
			return to & LITERAL;
		case INT_PTR:
			return to & (LITERAL | ADDRESSABLE);
		case NAME:
			return to & NAMED;
		case ALLOCA:
			return to & ADDRESSABLE;
	}
}

static val *val_need(val *v, enum val_to to)
{
	if(val_in(v, to))
		return v;
	assert(0);
}

unsigned val_hash(val *v)
{
	unsigned h = v->type;

	switch(v->type){
		case INT:
		case INT_PTR:
			h ^= v->u.i;
			break;
		case NAME:
			h ^= dynmap_strhash(v->u.name);
			break;
		case ALLOCA:
			break;
	}

	return h;
}

bool val_maybe_op(enum op op, val *l, val *r, int *res)
{
	if(l->type != INT || r->type != INT)
		return false;

	*res = op_exe(op, l->u.i, r->u.i);

	return true;
}

char *val_str(val *v)
{
	/* XXX: memleak */
	char buf[256];
	switch(v->type){
		case INT:
		case INT_PTR:
			snprintf(buf, sizeof buf, "%d", v->u.i);
			break;
		case NAME:
			snprintf(buf, sizeof buf, "%s", v->u.name);
			break;
		case ALLOCA:
			snprintf(buf, sizeof buf, "alloca-%d-%p",
					v->u.alloca_cnt, (void *)v);
			break;
	}
	return xstrdup(buf);
}

static val *val_new(enum val_type k)
{
	/* XXX: memleak */
	val *v = xcalloc(1, sizeof *v);
	v->type = k;
	return v;
}

static val *val_name_new(void)
{
	/* XXX: static */
	static int n;
	char buf[32];

	val *v = val_new(NAME);

	snprintf(buf, sizeof buf, "tmp.%d", n++);

	v->u.name = xstrdup(buf);

	return v;
}

val *val_new_i(int i)
{
	val *p = val_new(INT);
	p->u.i = i;
	return p;
}

val *val_new_ptr_from_int(int i)
{
	val *p = val_new_i(i);
	p->type = INT_PTR;
	return p;
}

val *val_alloca(int n)
{
	val *v = val_new(ALLOCA);
	v->u.alloca_cnt = n;
	return v;
}

void val_store(val *rval, val *lval)
{
	lval = val_need(lval, ADDRESSABLE);
	rval = val_need(rval, LITERAL | NAMED);

	isn_store(rval, lval);
}

val *val_load(val *v)
{
	val *named = val_name_new();

	v = val_need(v, ADDRESSABLE);

	isn_load(named, v);

	return named;
}

val *val_element(val *lval, int i)
{

}

val *val_add(val *a, val *b)
{
	val *named = val_name_new();

	isn_op(op_add, a, b, named);

	return named;
}
