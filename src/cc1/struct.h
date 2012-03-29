#ifndef STRUCT_H
#define STRUCT_H

struct struct_union_st
{
	char *spel; /* "<anon ...>" if anon */
	int anon : 1;
	int is_union : 1;
	decl **members;
};

int struct_union_size(struct_union_st *);

struct_union_st *struct_union_add( symtable *, char *spel, decl **members, int is_union);
struct_union_st *struct_union_find(symtable *, const char *spel);

decl *struct_union_member_find(struct_union_st *st, const char *spel, where *die_where);

#define struct_union_str(x) ((x)->is_union ? "union" : "struct")

#endif
