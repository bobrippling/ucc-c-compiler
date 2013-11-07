#ifndef STRUCT_ENUM_H
#define STRUCT_ENUM_H

typedef struct enum_member
{
	char *spel;
	expr *val; /* (expr *)-1 if not given */
	decl_attr *attr; /* enum { ABC __attribute(()) [ = ... ] }; */
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
	unsigned anon : 1,
	         got_membs : 1, /* true if we've had {} (optionally no members) */
	         foldprog : 2,
	         flexarr : 1,
	         contains_const : 1;

#define SUE_FOLDED_NO 0
#define SUE_FOLDED_PARTIAL 1
#define SUE_FOLDED_FULLY 2

	int align, size;

	sue_member **members;
};

#define sue_str_type(t) (t == type_struct \
                        ? "struct"        \
                        : t == type_union \
                        ? "union"         \
                        : "enum")

#define sue_str(x) sue_str_type((x)->primitive)

#define sue_nmembers(x) dynarray_count((x)->members)

#define sue_complete(sue) (\
		(sue)->got_membs && (sue)->foldprog == SUE_FOLDED_FULLY)

sue_member *sue_member_from_decl(decl *);

struct_union_enum_st *sue_find_descend(
		symtable *stab, const char *spel, int *descended);

struct_union_enum_st *sue_find_this_scope(symtable *, const char *spel);

/* we need to know if the struct is a definition at this point,
 * e.g.
 * struct A { int i; };
 * f()
 * {
 *   struct A a; // old type
 *   struct A;   // new type
 * }
 */
struct_union_enum_st *sue_decl(
		symtable *stab, char *spel,
		sue_member **members, enum type_primitive prim,
		int got_membs, int is_declaration);

/* enum specific */
void enum_vals_add(sue_member ***, char *, expr *, decl_attr *);
int  enum_nentries(struct_union_enum_st *);

void enum_member_search(enum_member **, struct_union_enum_st **, symtable *, const char *spel);

/* struct/union specific */
unsigned sue_size(struct_union_enum_st *, where *w);
unsigned sue_align(struct_union_enum_st *, where *w);
int sue_enum_size(struct_union_enum_st *st);

void sue_incomplete_chk(struct_union_enum_st *st, where *w);

decl *struct_union_member_find(struct_union_enum_st *,
		const char *spel, unsigned *extra_off,
		struct_union_enum_st **pin);
decl *struct_union_member_find_sue(struct_union_enum_st *, struct_union_enum_st *);

unsigned struct_union_member_offset(struct_union_enum_st *, const char *);

decl *struct_union_member_at_idx(struct_union_enum_st *, int idx); /* NULL if out of bounds */
int   struct_union_member_idx(struct_union_enum_st *, decl *);

#endif
