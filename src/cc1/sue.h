#ifndef STRUCT_ENUM_H
#define STRUCT_ENUM_H

#include "btype.h"

struct symtable;

typedef struct enum_member
{
	where where;
	char *spel;
	struct expr *val; /* (expr *)-1 if not given */
	struct attribute **attr; /* enum { ABC __attribute(()) [ = ... ] }; */
} enum_member;

typedef union sue_member
{
	struct decl        *struct_member;
	enum_member *enum_member;
} sue_member;

typedef struct struct_union_enum_st struct_union_enum_st;
struct struct_union_enum_st
{
	where where;
	struct attribute **attr;
	enum type_primitive primitive; /* struct or enum or union */

	char *spel; /* "<anon ...>" if anon */
	unsigned anon : 1,
	         membs_progress : 2, /* see SUE_MEMBS_* */
	         foldprog : 2,
	         flexarr : 1,
	         contains_const : 1;

#define SUE_FOLDED_NO 0
#define SUE_FOLDED_PARTIAL 1
#define SUE_FOLDED_FULLY 2

#define SUE_MEMBS_NO 0
#define SUE_MEMBS_PARSING 1
#define SUE_MEMBS_COMPLETE 2

	unsigned align, size;

	sue_member **members;
};

#define sue_str_type(t) (t == type_struct \
                        ? "struct"        \
                        : t == type_union \
                        ? "union"         \
                        : "enum")

#define sue_str(x) sue_str_type((x)->primitive)

#define sue_nmembers(x) dynarray_count((x)->members)

#define sue_is_complete(sue) (\
		(sue)->membs_progress == SUE_MEMBS_COMPLETE && (sue)->foldprog == SUE_FOLDED_FULLY)

sue_member *sue_member_from_decl(struct decl *);

struct_union_enum_st *sue_find_descend(
		struct symtable *stab, const char *spel, int *const descended);

struct_union_enum_st *sue_find_this_scope(
		struct symtable *stab, const char *spel);

struct_union_enum_st *sue_predeclare(
		struct symtable *stab,
		/*consumed*/char *spel,
		enum type_primitive prim /* S, U or E */,
		const where *)
	ucc_nonnull((1, 4));

void sue_define(struct_union_enum_st *sue, sue_member **members)
	ucc_nonnull();

void sue_member_init_dup_check(
		sue_member **members,
		enum type_primitive prim,
		const char *spel /* nullable */,
		where *sue_location);

sue_member *sue_drop(struct_union_enum_st *sue, sue_member **pos);

/* enum specific */
void enum_vals_add(sue_member ***, where *, char *,
		struct expr *, struct attribute **);

int enum_nentries(struct_union_enum_st *);

void enum_member_search_nodescend(
		enum_member **, struct_union_enum_st **,
		struct symtable *, const char *spel);

#ifdef NUM_H
int enum_has_value(struct_union_enum_st *, integral_t);
#endif

/* struct/union specific */
int sue_size(struct_union_enum_st *); /* -1 on error */
int sue_align(struct_union_enum_st *); /* -1 on error */

unsigned sue_size_assert(struct_union_enum_st *); /* aborts on error */

enum sue_szkind
{
	SUE_NORMAL,
	SUE_EMPTY,
	SUE_NONAMED
};
enum sue_szkind sue_sizekind(struct_union_enum_st *);

enum sue_anonextkind
{
	SUE_ANONEXT_ALLOW, /* -fms/plan9-extensions */
	SUE_ANONEXT_DENY, /* ms/plan9 extension attempted without -fflag */
	SUE_ANONEXT_ALLOW_C11, /* fine if in C11 */
};

enum sue_anonextkind sue_anonext_type(struct decl *, struct_union_enum_st *);

ucc_wur
int sue_emit_error_if_incomplete(struct_union_enum_st *st, const where *w);

struct decl *struct_union_member_find(struct_union_enum_st *,
		const char *spel, unsigned *extra_off,
		struct_union_enum_st **pin);
struct decl *struct_union_member_find_sue(struct_union_enum_st *, struct_union_enum_st *);

unsigned struct_union_member_offset(struct_union_enum_st *, const char *);

struct decl *struct_union_member_at_idx(struct_union_enum_st *, int idx); /* NULL if out of bounds */
int   struct_union_member_idx(struct_union_enum_st *, struct decl *);

#endif
