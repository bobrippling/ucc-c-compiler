#ifndef STRUCT_ENUM_H
#define STRUCT_ENUM_H

typedef struct enum_member
{
	char *spel;
	expr *val; /* (expr *)-1 if not given */
	int tag; /* for switch() checking */
} enum_member;

typedef union sue_member
{
	decl        struct_member;
	enum_member enum_member;
} sue_member;

struct struct_union_enum_st
{
	where where;
	enum type_primitive primitive;

	char *spel; /* "<anon ...>" if anon */
	int anon : 1;

	sue_member **members;
};


#define sue_str(x)  ((x)->primitive == type_struct \
																	? "struct"                     \
																	: (x)->primitive == type_union \
																	? "union"                      \
																	: "enum")

/* this is fine - empty structs aren't allowed */
#define sue_incomplete(x) (!(x)->members)

#define sue_nmembers(x) dynarray_count((void **)(x)->members)


struct_union_enum_st *sue_add( symtable *, char *spel, sue_member **members, enum type_primitive);
struct_union_enum_st *sue_find(symtable *, const char *spel);

void sue_set_spel(char **dest, char *spel, const char *desc);

/* enum specific */
void enum_vals_add(sue_member ***, char *, expr *);
int  enum_nentries(struct_union_enum_st *);

void enum_member_search(enum_member **, struct_union_enum_st **, symtable *, const char *spel);

/* struct/union specific */
int struct_union_size(struct_union_enum_st *);

decl *struct_union_member_find(struct_union_enum_st *, const char *spel, where *die_where);

#endif
