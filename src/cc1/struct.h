#ifndef STRUCT_H
#define STRUCT_H

struct struct_st
{
	char *spel; /* "<anon ...>" if anon */
	int anon;
	decl **members;
};

int struct_size(struct_st *);

struct_st *struct_add( symtable *, char *spel, decl **members);
struct_st *struct_find(symtable *, const char *spel);
decl *struct_member_find(struct_st *st, const char *spel, where *die_where);

#endif
