#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

#include "mem.h"

#include "backend.h"

struct val
{
	enum val_type
	{
		INT,
		INT_PTR,
		NAME
	} type;

	union
	{
		int i;
		char *name;
	} bits;
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
	}
}

static val *val_need(val *v, enum val_to to)
{
	if(val_in(v, to))
		return v;
	assert(0);
}

static char *val_str(val *v)
{
	/* XXX: memleak */
	char buf[256];
	switch(v->type){
		case INT:
		case INT_PTR:
			snprintf(buf, sizeof buf, "%d", v->bits.i);
			break;
		case NAME:
			snprintf(buf, sizeof buf, "%s", v->bits.name);
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

	v->bits.name = xstrdup(buf);

	return v;
}

val *val_new_i(int i)
{
	val *p = val_new(INT);
	p->bits.i = i;
	return p;
}

val *val_new_ptr_from_int(int i)
{
	val *p = val_new_i(i);
	p->type = INT_PTR;
	return p;
}

static void assert_address(val *v)
{
	assert(v->type == INT_PTR);
}

void val_store(val *d, val *s)
{
	d = val_need(d, ADDRESSABLE);
	s = val_need(s, LITERAL | NAMED);

	isn_store(STORE, d, s);
	printf("store %s -> i32* %s\n", val_str(s), val_str(d));
}

val *val_load(val *v)
{
	val *named = val_name_new();

	v = val_need(v, ADDRESSABLE);

	printf("load i32* %s -> %s\n", val_str(v), val_str(named));

	return named;
}

val *val_add(val *a, val *b)
{
	val *named = val_name_new();

	printf("%s <- %s + %s\n", val_str(named), val_str(a), val_str(b));

	return named;
}

val *val_show(val *v)
{
	printf("SHOW: %s\n", val_str(v));
	return v;
}
