#include <stdlib.h>

#include "alloc.h"
#include "tree.h"

extern int currentline, currentchar;

tree *tree_new()
{
	tree *t = umalloc(sizeof *t);
	t->where.line = currentline;
	t->where.chr  = currentchar;
	return t;
}

expr *expr_new()
{
	expr *e = umalloc(sizeof *e);
	e->where.line = currentline;
	e->where.chr  = currentchar;
	return e;
}
