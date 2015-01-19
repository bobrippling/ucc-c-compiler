#include <stdio.h>

#include "mem.h"
#include "dynmap.h"

#include "backend.h"
#include "val_internal.h"
#include "isn.h"

typedef struct isn isn;

struct isn
{
	enum isn_type
	{
		ISN_LOAD,
		ISN_STORE,
		ISN_ALLOCA,
		ISN_OP,
		ISN_ELEM
	} type;

	union
	{
		struct
		{
			val *lval, *to;
		} load;
		struct
		{
			val *lval, *from;
		} store;

		struct
		{
			enum op op;
			val *lhs, *rhs, *res;
		} op;

		struct
		{
			val *lval, *add, *res;
		} elem;

		struct
		{
			unsigned sz;
			val *out;
		} alloca;
	} u;

	isn *next;
};

static isn *head, **tail = &head;

static isn *isn_new(enum isn_type t)
{
	isn *isn = xcalloc(1, sizeof *isn);

	*tail = isn;
	tail = &isn->next;

	isn->type = t;
	return isn;
}

void isn_load(val *to, val *lval)
{
	isn *isn = isn_new(ISN_LOAD);

	isn->u.load.lval = lval;
	isn->u.load.to = to;
}

void isn_store(val *from, val *lval)
{
	isn *isn = isn_new(ISN_STORE);

	isn->u.store.lval = lval;
	isn->u.store.from = from;
}

void isn_op(enum op op, val *lhs, val *rhs, val *res)
{
	isn *isn = isn_new(ISN_OP);
	isn->u.op.op = op;
	isn->u.op.lhs = lhs;
	isn->u.op.rhs = rhs;
	isn->u.op.res = res;
}

void isn_elem(val *lval, val *add, val *res)
{
	isn *isn = isn_new(ISN_ELEM);
	isn->u.elem.lval = lval;
	isn->u.elem.add = add;
	isn->u.elem.res = res;
}

void isn_alloca(unsigned sz, val *v)
{
	isn *isn = isn_new(ISN_ALLOCA);
	isn->u.alloca.sz = sz;
	isn->u.alloca.out = v;
}

static val *resolve_val(val *initial, dynmap *stores2rvals)
{
	val *resolved = dynmap_get(val *, val *, stores2rvals, initial);

	if(resolved){
		printf("# resolved %s -> %s\n", val_str(initial), val_str(resolved));
		return resolved;
	}
	return initial;
}

static val *ret_val(val *initial, dynmap *stores2rvals)
{
	return initial;
}

void isn_dump(bool optimise)
{
	dynmap *stores2rvals = dynmap_new(val *, /*ref*/NULL, val_hash);
	val *(*const resolve)(val *, dynmap *) = optimise ? resolve_val : ret_val;
	isn *i;

	for(i = head; i; i = i->next){
		switch(i->type){
			case ISN_STORE:
			{
				val *resolved_rval = resolve(i->u.store.from, stores2rvals);

				dynmap_set(val *, val *,
						stores2rvals,
						i->u.store.lval, resolved_rval);

				printf("\tstore %s, %s\n",
							val_str(i->u.load.lval),
							val_str(resolved_rval));
				break;
			}

			case ISN_LOAD:
			{
				val *rval = resolve(i->u.load.lval, stores2rvals);

				printf("\t%s = load %s\n",
						val_str(i->u.load.to),
						val_str(rval));

				dynmap_set(val *, val *,
						stores2rvals,
						i->u.load.to, rval);
				break;
			}

			case ISN_ALLOCA:
			{
				printf("\t%s = alloca %u\n",
						val_str(i->u.alloca.out),
						i->u.alloca.sz);
				break;
			}

			case ISN_ELEM:
			{
				val *solved_lval = i->u.elem.lval;
				/* ^ lval - doesn't resolve since we don't want the value inside it */
				val *solved_add = resolve(i->u.elem.add, stores2rvals);
				int res;

				printf("\t%s = elem %s, %s\n",
							val_str(i->u.elem.res),
							val_str(solved_lval),
							val_str(solved_add));
				break;
			}

			case ISN_OP:
			{
				val *solved_lhs = resolve(i->u.op.lhs, stores2rvals);
				val *solved_rhs = resolve(i->u.op.rhs, stores2rvals);
				int res;

				printf("\t%s = %s %s, %s\n",
						val_str(i->u.op.res),
						"+",
						val_str(solved_lhs),
						val_str(solved_rhs));

				if(val_maybe_op(i->u.op.op, solved_lhs, solved_rhs, &res)){
					val *synth_add = val_new_i(res);

					dynmap_set(val *, val *,
							stores2rvals,
							i->u.op.res, synth_add);
				}
				break;
			}
		}
	}
}
