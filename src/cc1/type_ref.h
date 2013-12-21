#ifndef TYPE_REF_H
#define TYPE_REF_H

struct type_ref
{
	where where;
	type_ref *ref, *tmp; /* tmp used for things like printing */

	decl_attr *attr;
	int folded;

	enum type_ref_type
	{
		type_ref_type,  /* end - at type */
		type_ref_tdef,  /* type reference to next ref */
		type_ref_ptr,   /* pointer to next ref */
		type_ref_block, /* block pointer to next ref (func) */
		type_ref_func,  /* function */
		type_ref_array, /* array of next ref, similar to pointer */
		type_ref_cast   /* used for adding qualifiers */
	} type;

	union
	{
		/* ref_type */
		const type *type;

		/* ref_tdef */
		struct type_ref_tdef
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
type_ref_cmp(
		type_ref *, type_ref *,
		enum type_cmp_opts);

#endif
