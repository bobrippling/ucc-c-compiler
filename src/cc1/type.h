#ifndef TYPE_H
#define TYPE_H

#include "num.h"
#include "btype.h"

typedef struct type type;

enum vla_kind
{
	VLA_NO,
	VLA,
	VLA_STAR
};

struct type
{
	type *ref, *tmp; /* tmp used for things like printing */

	int folded;

	struct type_tree *uptree;

	enum type_kind
	{
		type_btype,  /* end - at type */
		type_tdef,  /* type reference to next ref */
		type_ptr,   /* pointer to next ref */
		type_block, /* block pointer to next ref (func) */
		type_func,  /* function */
		type_array, /* array of next ref, similar to pointer */
		type_auto, /* to be inferred */
		type_cast,  /* used for adding qualifiers */
		type_attr,  /* __attribute__ */
		type_where  /* .where */
#define N_TYPE_KINDS (type_where + 1)
#define case_CONCRETE_TYPE \
			case type_btype:     \
			case type_ptr:       \
			case type_block:     \
			case type_func:      \
			case type_array

	} type;

	union
	{
		/* type_btype */
		const btype *type;

		/* type_attr */
		struct attribute **attr;

		/* type_tdef */
		struct type_tdef
		{
			/* never null - expression whose type is the typedef or typeof type */
			struct expr *type_of;
			/* null for typeof. non-null means it's the typedef decl */
			struct decl *decl;
		} tdef;

		/* type_array */
		struct
		{
			struct expr *size;
			/* when we decay
			 * f(int x[2]) -> f(int *x)
			 * we save the size + is_static
			 */
			unsigned is_static : 1;
			enum vla_kind vla_kind;
		} array;

		struct
		{
			/* if null, this is just a normal pointer */
			struct type *decayed_from;
		} ptr;

		/* type_cast */
		struct
		{
			enum type_qualifier qual;
		} cast;

		/* type_func */
		struct
		{
			struct funcargs *args;
			struct symtable *arg_scope;
		} func;

		/* type_block */
		/* nothing */

		/* type_where */
		where where;
	} bits;
};


enum type_cmp_opts
{
	TYPE_CMP_ALLOW_TENATIVE_ARRAY = 1 << 0,
};

enum type_cmp
type_cmp(
		type *, type *,
		enum type_cmp_opts);

int type_eq_nontdef(type *, type *);

unsigned type_size(type *r, where const *from);
unsigned type_align(type *r, where const *from);
unsigned type_align_no_attr(type *r, where const *from);

const char *type_kind_to_str(enum type_kind);

#define TYPE_STATIC_BUFSIZ 512

const char *type_to_str_r_spel(
		char buf[TYPE_STATIC_BUFSIZ],
		type *r, const char *spel);

const char *type_to_str_r_show_decayed(
		char buf[TYPE_STATIC_BUFSIZ], type *r);

const char *type_to_str_r(char buf[TYPE_STATIC_BUFSIZ], type *r);

const char *type_to_str(type *r);

/* char[] and char *, etc */
enum type_str_type
{
	type_str_no,
	type_str_char,
	type_str_wchar
};
enum type_str_type type_str_type(type *);

integral_t type_max(type *r, where *from);
sintegral_t type_min(type *r, where *from);

where *type_loc(type *);
where *type_has_loc(type *);

unsigned type_hash(const type *);
unsigned type_hash_skip_nontdefs_consts(const type *);
unsigned sue_hash(const struct struct_union_enum_st *);

/* returns the largest type to hold a type of size 'sz' */
enum type_primitive type_primitive_not_less_than_size(unsigned sz, int is_signed);

#endif
