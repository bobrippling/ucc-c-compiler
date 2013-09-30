#ifndef TYPE_H
#define TYPE_H

enum type_cmp
{
	TYPE_EQUAL,
	TYPE_CONVERTIBLE_IMPLICIT,
	TYPE_CONVERTIBLE_EXPLICIT,
	TYPE_NOT_EQUAL,
	TYPE_QUAL_LOSS
};

enum type_primitive
{
	type_void,
	type__Bool,
#define type_wchar (platform_sys() == PLATFORM_CYGWIN ? type_short : type_int)

	/* signed, unsigned and 'normal' */
	type_nchar,
	type_schar,
	type_uchar,

	/* unsigned primitive is signed primitive + 1 */
#define TYPE_PRIMITIVE_TO_UNSIGNED(p) ((p) + 1)
#define S_U_TY(nam) type_ ## nam, type_u ## nam

	S_U_TY(int),
	S_U_TY(short),
	S_U_TY(long),
	S_U_TY(llong),

	type_float,
	type_double,
	type_ldouble,

	type_struct,
	type_union,
	type_enum,

	type_unknown
};

enum type_qualifier
{
	qual_none     = 0,
	qual_const    = 1 << 0,
	qual_volatile = 1 << 1,
	qual_restrict = 1 << 2,
};

typedef struct type type;

struct type
{
	where where;

	enum type_primitive primitive;

	/* NULL unless this is a struct, union or enum */
	struct_union_enum_st *sue;

	/* attr applied to all decls whose type is this type */
	decl_attr *attr;
};

enum type_cmp type_cmp(const type *a, const type *b);
int type_is_signed(const type *);

/* is there a loss of qualifiers going from 'b' to 'a' ? */
int type_qual_loss(enum type_qualifier a, enum type_qualifier b);

#endif
