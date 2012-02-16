#ifndef STRUCT_H
#define STRUCT_H

struct struct_st
{
	char *spel; /* NULL if anon */
	decl **members;
};

int struct_size(struct_st *);

struct_st *struct_add( struct_st ***structs, char *spel, decl **members);
struct_st *struct_find(struct_st **structs, const char *spel);

#endif
