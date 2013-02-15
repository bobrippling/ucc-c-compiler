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
	decl        *struct_member;
	enum_member *enum_member;
} sue_member;

struct struct_union_enum_st
{
	where where;
	decl_attr *attr;
	enum type_primitive primitive; /* struct or enum or union */

	char *spel; /* "<anon ...>" if anon */
	int anon : 1;
	int align, size;

	sue_member **members;
};

#define sue_str_type(t) (t == type_struct \
                        ? "struct"        \
                        : t == type_union \
                        ? "union"         \
                        : "enum")

#define sue_str(x) sue_str_type((x)->primitive)

/* this is fine - empty structs aren't allowed */
#define sue_incomplete(x) (!(x)->members)

#define sue_nmembers(x) dynarray_count((void **)(x)->members)

sue_member *sue_member_from_decl(decl *);

struct_union_enum_st *sue_add( symtable *, char *spel, sue_member **members, enum type_primitive);
struct_union_enum_st *sue_find(symtable *, const char *spel);

/* enum specific */
void enum_vals_add(sue_member ***, char *, expr *);
int  enum_nentries(struct_union_enum_st *);

void enum_member_search(enum_member **, struct_union_enum_st **, symtable *, const char *spel);

/* struct/union specific */
int sue_size(struct_union_enum_st *, const where *w);

decl *struct_union_member_find(struct_union_enum_st *, const char *spel, unsigned *extra_off);
decl *struct_union_member_find_sue(struct_union_enum_st *, struct_union_enum_st *);

decl *struct_union_member_at_idx(struct_union_enum_st *, int idx); /* NULL if out of bounds */
int   struct_union_member_idx(struct_union_enum_st *, decl *);

#endif
