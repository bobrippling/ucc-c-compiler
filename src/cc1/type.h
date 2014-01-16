#ifndef TYPE_H
#define TYPE_H

struct type
{
	where where;
	type *ref, *tmp; /* tmp used for things like printing */

	decl_attr *attr;
	int folded;

	enum type_type
	{
		type_type,  /* end - at type */
		type_tdef,  /* type reference to next ref */
		type_ptr,   /* pointer to next ref */
		type_block, /* block pointer to next ref (func) */
		type_func,  /* function */
		type_array, /* array of next ref, similar to pointer */
		type_cast   /* used for adding qualifiers */
	} type;

	union
	{
		/* ref_type */
		const btype *type;

		/* ref_tdef */
		struct type_tdef
		{
			expr *type_of;
			decl *decl;
		} tdef;

		/* ref_{ptr,array} */
		struct
		{
			enum type_qualifier qual;
			unsigned is_static : 1;
			unsigned decayed : 1; /* old size may be NULL - track here */
			expr *size;
			/* when we decay
			 * f(int x[2]) -> f(int *x)
			 * we save the size + is_static
			 */
		} ptr, array;

		/* ref_cast */
		struct
		{
			char is_signed_cast; /* if true - signed_true else qual */
			char signed_true;
			char additive; /* replace qual or add? */
			enum type_qualifier qual;
		} cast;

		/* ref_func */
		struct
		{
			struct funcargs *args;
			symtable *arg_scope;
		} func;

		/* ref_block */
		struct
		{
			struct funcargs *func;
			enum type_qualifier qual;
		} block;
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

#endif
