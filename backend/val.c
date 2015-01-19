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
		NAME_LVAL,
		ALLOCA
	} type;

	union
	{
		int i;
		struct
		{
			union
			{
				char *name;
				struct
				{
					unsigned bytesz;
					int idx;
				} alloca;
			} u;
			struct val_idxpair
			{
				val *val;
				unsigned idx;
				struct val_idxpair *next;
			} *idxpair;
			/* val* => val*
			 * where .first is a INT */
		} addr;
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
		case NAME_LVAL:
			return to & ADDRESSABLE;
		case ALLOCA:
			return to & ADDRESSABLE;
	}
}

static val *val_need(val *v, enum val_to to, const char *from)
{
	if(val_in(v, to))
		return v;

	fprintf(stderr, "%s: val_need(): don't have 0x%x\n", from, to);
	assert(0);
}
#define VAL_NEED(v, t) val_need(v, t, __func__)

unsigned val_hash(val *v)
{
	unsigned h = v->type;

	switch(v->type){
		case INT:
		case INT_PTR:
			h ^= v->u.i;
			break;
		case NAME_LVAL:
		case NAME:
			h ^= dynmap_strhash(v->u.addr.u.name);
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
		case NAME_LVAL:
			snprintf(buf, sizeof buf, "%s", v->u.addr.u.name);
			break;
		case ALLOCA:
			snprintf(buf, sizeof buf, "alloca.%d", v->u.addr.u.alloca.idx);
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

static val *val_name_new_lval_(bool lval)
{
	/* XXX: static */
	static int n;
	char buf[32];

	val *v = val_new(lval ? NAME_LVAL : NAME);

	snprintf(buf, sizeof buf, "tmp.%d", n++);

	v->u.addr.u.name = xstrdup(buf);

	return v;
}

static val *val_name_new(void)
{
	return val_name_new_lval_(false);
}

static val *val_name_new_lval(void)
{
	return val_name_new_lval_(true);
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

val *val_alloca(int n, unsigned elemsz)
{
	/* XXX: static */
	static int idx;
	const unsigned bytesz = n * elemsz;
	val *v = val_new(ALLOCA);

	isn_alloca(bytesz, v);

	v->u.addr.u.alloca.bytesz = bytesz;
	v->u.addr.u.alloca.idx = idx++;

	return v;
}

void val_store(val *rval, val *lval)
{
	lval = VAL_NEED(lval, ADDRESSABLE);
	rval = VAL_NEED(rval, LITERAL | NAMED);

	isn_store(rval, lval);
}

val *val_load(val *v)
{
	val *named = val_name_new();

	v = VAL_NEED(v, ADDRESSABLE);

	isn_load(named, v);

	return named;
}

static val *val_alloca_idx(val *lval, unsigned idx)
{
	lval = VAL_NEED(lval, ADDRESSABLE);

	for(struct val_idxpair *pair = lval->u.addr.idxpair;
			pair;
			pair = pair->next)
	{
		if(pair->idx == idx)
			return pair->val;
	}

	return NULL;
}

static void val_alloca_idx_set(val *lval, unsigned idx, val *elemptr)
{
	lval = VAL_NEED(lval, ADDRESSABLE);

	struct val_idxpair **next;
	for(next = &lval->u.addr.idxpair;
			*next;
			next = &(*next)->next)
	{
		assert((*next)->idx == idx);
	}

	struct val_idxpair *newpair = xcalloc(1, sizeof *newpair);
	*next = newpair;
	newpair->val = elemptr;
	newpair->idx = idx;
}

val *val_element(val *lval, int i, unsigned elemsz)
{
	const unsigned byteidx = i * elemsz;

	val *pre_existing = val_alloca_idx(lval, byteidx);
	if(pre_existing)
		return pre_existing;

	val *named = val_name_new_lval();
	val *vidx = val_new_i(byteidx);

	isn_elem(lval, vidx, named);

	val_alloca_idx_set(lval, byteidx, named);

	return named;
}

val *val_add(val *a, val *b)
{
	val *named = val_name_new();

	isn_op(op_add, a, b, named);

	return named;
}
