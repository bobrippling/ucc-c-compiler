#ifndef STRUCT_H
#define STRUCT_H

struct struct_st
{
	char *spel; /* NULL if anon */
	decl **members;
};

int struct_size(struct_st *);

struct_st *struct_add( symtable *, char *spel, decl **members);
struct_st *struct_find(symtable *, const char *spel);

#endif
