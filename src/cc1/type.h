#ifndef TYPE_H
#define TYPE_H

#include "num.h"
#include "btype.h"

typedef struct type type;

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
	} type;

	union
	{
		/* type_btype */
		const btype *type;

		/* type_attr */
		struct attribute *attr;

		/* type_tdef */
		struct type_tdef
		{
			/* never null - expression whose type is the typedef or typeof type */
			struct expr *type_of;
			/* null for typeof. non-null means it's the typedef decl */
			struct decl *decl;
		} tdef;

		/* type_{ptr,array} */
		struct
		{
			unsigned is_static : 1;
			unsigned decayed : 1; /* old size may be NULL - track here */
			unsigned is_vla : 2;
			enum
			{
				VLA = 1, VLA_STAR = 2
			};
			struct expr *size;
			/* when we decay
			 * f(int x[2]) -> f(int *x)
			 * we save the size + is_static
			 */
		} ptr, array;

		/* type_cast */
		struct
		{
			char is_signed_cast; /* if true - signed_true else qual */
			char signed_true;
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

unsigned type_size(type *r, where *from);
unsigned type_align(type *r, where *from);

const char *type_kind_to_str(enum type_kind);

#define TYPE_STATIC_BUFSIZ 512

const char *type_to_str_r_spel(
		char buf[TYPE_STATIC_BUFSIZ],
		type *r, char *spel);

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

where *type_loc(type *);
int type_has_loc(type *);

#endif
