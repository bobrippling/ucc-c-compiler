#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/alloc.h"

#include "label.h"

#include "out/lbl.h"

label *label_new(where *w, char *fn, char *id, int complete)
{
	label *l = umalloc(sizeof *l);
	l->pw = w;
	l->spel = id;
	l->mangled = out_label_goto(fn, id);
	l->complete = complete;
	return l;
}
