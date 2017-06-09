#ifndef BTYPE_H
#define BTYPE_H

#include "../util/where.h"

enum type_cmp
{
	TYPE_EQUAL = 1 << 0,
	TYPE_EQUAL_TYPEDEF = 1 << 1, /* size_t <-> unsigned long */

	TYPE_QUAL_ADD = 1 << 2, /* const int <- int */
	TYPE_QUAL_SUB = 1 << 3, /* int <- const int */
	TYPE_QUAL_POINTED_ADD = 1 << 4, /* const char * <- char * */
	TYPE_QUAL_POINTED_SUB = 1 << 5, /* char * <- const char * */
	TYPE_QUAL_NESTED_CHANGE = 1 << 6, /* char ** <- const char ** */

	TYPE_CONVERTIBLE_IMPLICIT = 1 << 7,
	TYPE_CONVERTIBLE_EXPLICIT = 1 << 8,

	TYPE_NOT_EQUAL = 1 << 9
};
#define TYPE_EQUAL_ANY (TYPE_EQUAL | TYPE_EQUAL_TYPEDEF)

const char *type_cmp_to_str(enum type_cmp);

enum type_primitive
{
	type_void,
	type__Bool,
#define type_wchar (platform_sys() == PLATFORM_CYGWIN ? type_short : type_int)

	/* signed, unsigned and 'normal' */
	type_nchar,
	type_schar,
	type_uchar,

	/* unsigned primitive is signed primitive + 1
	 * this is important for type_intrank() */
#define TYPE_PRIMITIVE_TO_UNSIGNED(p) ((p) + 1)
#define TYPE_PRIMITIVE_TO_SIGNED(p) ((p) - 1)
#define TYPE_PRIMITIVE_IS_CHAR(a) (type_nchar <= (a) && (a) <= type_uchar)
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
#define type_intptr_t type_long
#define type_uintptr_t type_ulong

enum type_qualifier
{
	qual_none     = 0,
	qual_const    = 1 << 0,
	qual_volatile = 1 << 1,
	qual_restrict = 1 << 2,

	qual_nullable = 1 << 3,
	qual_nonnull  = 1 << 4,
};

typedef struct btype btype;

struct btype
{
	enum type_primitive primitive;

	/* NULL unless this is a struct, union or enum */
	/* special case - if we're an int, this may refer to the enum the int came from */
	struct struct_union_enum_st *sue;
};

enum type_cmp btype_cmp(const btype *a, const btype *b);
int type_primitive_is_signed(enum type_primitive, int hard_err_on_su);
int btype_is_signed(const btype *);

int type_intrank(enum type_primitive);

#define BTYPE_STATIC_BUFSIZ 128
const char *btype_to_str(const btype *t);
unsigned btype_size( const btype *, where const *from);
unsigned btype_align(const btype *, where const *from);

const char *type_primitive_to_str(const enum type_primitive);
const char *type_qual_to_str(     const enum type_qualifier, int trailing_space);

int type_floating(enum type_primitive);
unsigned type_primitive_size(enum type_primitive tp);
unsigned long long
type_primitive_max(enum type_primitive p, int is_signed);

#endif
