#include <stdlib.h>
#include <stdarg.h>

#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/where.h"

#include "snapshot.h"
#include "macro.h"

#define NEW(exp) umalloc(sizeof *exp)
#define NEW_N(n, exp) umalloc(n * sizeof *exp)

struct snapshot
{
	int *pre, *post;
	size_t n;
};

static int *snapshot_take_1(size_t n)
{
	int *p;
	size_t i;

	if(!n)
		return NULL;

	p = NEW_N(n, p);

	for(i = 0; i < n; i++)
		p[i] = macros[i]->use_cnt;

	return p;
}

void snapshot_restore_used(snapshot *snap)
{
	size_t i;
	for(i = 0; i < snap->n; i++)
		macros[i]->use_cnt = snap->pre[i];
}

snapshot *snapshot_take(void)
{
	snapshot *snap;
	size_t n = dynarray_count(macros);

	snap       = NEW(snap);
	snap->n    = n;
	snap->pre  = snapshot_take_1(snap->n);

	return snap;
}

void snapshot_free(snapshot *snap)
{
	free(snap->pre);
	free(snap->post);
	free(snap);
}

void snapshot_take_post(snapshot *snap)
{
	snap->post = snapshot_take_1(snap->n);
}

static void snapshot_alter_blue(snapshot *snap, int change)
{
	size_t i;

	for(i = 0; i < snap->n; i++)
		if(snap->post[i] != snap->pre[i])
#ifdef EVAL_DEBUG
			printf(">> blue %s += %d\n", macros[i]->nam, change),
#endif
			macros[i]->blue += change;
}

void snapshot_blue_used(snapshot *snap)
{
	snapshot_alter_blue(snap, 1);
}

void snapshot_unblue_used(snapshot *snap)
{
	snapshot_alter_blue(snap, -1);
}
