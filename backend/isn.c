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
		ISN_OP
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

static val *resolve_val(val *initial, dynmap *stores2rvals)
{
	val *resolved = dynmap_get(val *, val *, stores2rvals, initial);

	if(resolved){
		printf("# resolved %s -> %s\n", val_str(initial), val_str(resolved));
		return resolved;
	}
	return initial;
}

void isn_optimise()
{
	dynmap *stores2rvals = dynmap_new(val *, /*ref*/NULL, val_hash);
	isn *i;

	for(i = head; i; i = i->next){
		switch(i->type){
			case ISN_STORE:
			{
				val *resolved_rval = resolve_val(i->u.store.from, stores2rvals);

				dynmap_set(val *, val *,
						stores2rvals,
						i->u.store.lval, resolved_rval);

				printf("store %s -> %s\n",
							val_str(resolved_rval),
							val_str(i->u.load.lval));
				break;
			}

			case ISN_LOAD:
			{
				val *rval = resolve_val(i->u.load.lval, stores2rvals);

				printf("load %s -> %s\n",
						val_str(rval),
						val_str(i->u.load.to));

				dynmap_set(val *, val *,
						stores2rvals,
						i->u.load.to, rval);
				break;
			}

			case ISN_OP:
			{
				val *solved_lhs = resolve_val(i->u.op.lhs, stores2rvals);
				val *solved_rhs = resolve_val(i->u.op.rhs, stores2rvals);
				int res;

				printf("op: %s %s %s\n",
							val_str(solved_lhs),
							"+",
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

/*
void isn_dump()
{
	isn *i;
	for(i = head; i; i = i->next){
		switch(i->type){
			case ISN_LOAD:
				printf("load: %s <- %s\n",
						val_str(i->u.load.to),
						val_str(i->u.load.lval));
				break;

			case ISN_STORE:
				printf("store: %s <- %s\n",
						val_str(i->u.store.lval),
						val_str(i->u.store.from));
				break;

			case ISN_OP:
				printf("op: %s %s %s\n",
						val_str(i->u.op.lhs),
						"+",
						val_str(i->u.op.rhs));
				break;
		}
	}
}
*/
