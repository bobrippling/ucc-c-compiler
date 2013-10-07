#include <stdlib.h>
#include <stdarg.h>

#include "../util/where.h"
#include "../util/alloc.h"

#include "label.h"

label *label_new(where *w, char *id, int complete)
{
	label *l = umalloc(sizeof *l);
	l->pw = w;
	l->spel = id;
	l->complete = complete;
	return l;
}
