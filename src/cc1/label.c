#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/alloc.h"

#include "sym.h"

#include "label.h"

#include "out/out.h"

label *label_new(where *w, char *id, int complete, symtable *scope)
{
	label *l = umalloc(sizeof *l);
	l->pw = w;
	l->spel = id;
	l->complete = complete;
	l->scope = scope;
	return l;
}

void label_makeblk(label *l, out_ctx *octx)
{
	if(l->bblock)
		return;
	l->bblock = out_blk_new(octx, l->spel);
}
