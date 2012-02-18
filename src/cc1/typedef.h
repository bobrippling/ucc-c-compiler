#ifndef TYPEDEF_H
#define TYPEDEF_H

struct tdef
{
	decl *decl;
	tdef *next;
};

struct tdeftable
{
	tdef *first;
	tdeftable *parent;
};

decl *typedef_find(struct tdeftable *defs, const char *spel);
void  typedef_add( struct tdeftable *defs, decl *d);

#endif
