#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/alloc.h"

#include "label.h"

#include "out/out.h"

label *label_new(where *w, char *fn, char *id, int complete)
{
	label *l = umalloc(sizeof *l);
	l->pw = w;
	l->spel = id;
	(void)fn;
	l->bblock = out_blk_new(id);
	l->complete = complete;
	return l;
}
