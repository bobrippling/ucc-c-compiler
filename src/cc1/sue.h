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
	unsigned anon : 1, complete : 1;
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
#define sue_incomplete(x) (!(x)->complete)

#define sue_nmembers(x) dynarray_count((x)->members)

sue_member *sue_member_from_decl(decl *);

struct_union_enum_st *sue_find_or_add(symtable *, char *spel, sue_member **members, enum type_primitive, int complete);
struct_union_enum_st *sue_find_this_scope(symtable *, const char *spel);

/* enum specific */
void enum_vals_add(sue_member ***, char *, expr *);
int  enum_nentries(struct_union_enum_st *);

void enum_member_search(enum_member **, struct_union_enum_st **, symtable *, const char *spel);

/* struct/union specific */
int sue_size(struct_union_enum_st *, where *w);
int sue_enum_size(struct_union_enum_st *st);

decl *struct_union_member_find(struct_union_enum_st *,
		const char *spel, unsigned *extra_off,
		struct_union_enum_st **pin);
decl *struct_union_member_find_sue(struct_union_enum_st *, struct_union_enum_st *);

unsigned struct_union_member_offset(struct_union_enum_st *, const char *);

decl *struct_union_member_at_idx(struct_union_enum_st *, int idx); /* NULL if out of bounds */
int   struct_union_member_idx(struct_union_enum_st *, decl *);

#endif
