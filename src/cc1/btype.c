#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "../util/where.h"
#include "../util/util.h"
#include "../util/platform.h"
#include "../util/limits.h"

#include "macros.h"

#include "expr.h"
#include "sue.h"
#include "btype.h"
#include "cc1.h"
#include "fopt.h"

/* check special case: char */
ucc_static_assert(a, type_nchar < type_schar);
ucc_static_assert(b, type_schar < type_uchar);

/* rest follow on from type_int */
ucc_static_assert(c, type_int < type_uint);

static int type_convertible(enum type_primitive p)
{
	switch(p){
		case type__Bool:
		case type_nchar: case type_schar: case type_uchar:
		case type_int:   case type_uint:
		case type_short: case type_ushort:
		case type_long:  case type_ulong:
		case type_llong: case type_ullong:
		case type_float:
		case type_double:
		case type_ldouble:
		case type_enum:
			return 1;

		case type_void:
		case type_struct:
		case type_union:
		case type_unknown:
			break;
	}
	return 0;
}

enum type_cmp btype_cmp(const btype *a, const btype *b)
{
	switch(a->primitive){
		case type_void:
			/* allow (void) casts */
			return b->primitive == type_void
				? TYPE_EQUAL
				: TYPE_CONVERTIBLE_IMPLICIT;

		case type_struct:
		case type_union:
			return a->sue == b->sue ? TYPE_EQUAL : TYPE_NOT_EQUAL;

		case type_unknown:
			ICE("type unknown in %s", __func__);

		case type_enum:
			if(a->sue == b->sue)
				return TYPE_EQUAL;
			break; /* convertible check */

		default:
			if(a->primitive == b->primitive)
				return TYPE_EQUAL;
			break; /* convertible check */
	}

	/* only reachable for scalars and enums */
	if(type_convertible(a->primitive) && type_convertible(b->primitive))
		return TYPE_CONVERTIBLE_IMPLICIT;

	return TYPE_NOT_EQUAL;
}

int type_primitive_is_signed(enum type_primitive p, int hard_err_on_su)
{
	switch(p){
		case type_nchar:
			/* XXX: note we treat char as signed */
			/* -fsigned-char */
			return !!(cc1_fopt.signed_char);

		case type_schar:
		case type_int:
		case type_short:
		case type_long:
		case type_llong:
		case type_float:
		case type_double:
		case type_ldouble:
			return 1;

		case type_struct:
		case type_union:
			if(hard_err_on_su){
				ICE("%s(%s)",
						__func__,
						type_primitive_to_str(p));
			}
			return 0;

		case type_enum:
			return 0; /* for now - enum types coming later */

		case type_void:
		case type__Bool:
		case type_uchar:
		case type_uint:
		case type_ushort:
		case type_ulong:
		case type_ullong:
			return 0;

		case type_unknown:
			ICE("type_unknown in %s", __func__);
	}

	ucc_unreach(0);
}

int btype_is_signed(const btype *t)
{
	return type_primitive_is_signed(t->primitive, 1);
}

int type_intrank(enum type_primitive p)
{
	switch(p){
		default:
			return -1;

		case type_nchar:
		case type_schar:
		case type_uchar:
		case type_int:
		case type_uint:
		case type_short:
		case type_ushort:
		case type_long:
		case type_ulong:
		case type_llong:
		case type_ullong:
			return p;
	}
}

unsigned btype_size(const btype *t, const where *from)
{
	if(t->sue && t->primitive != type_int)
		return sue_size(t->sue, from);

	return type_primitive_size(t->primitive);
}

unsigned btype_align(const btype *t, const where *from)
{
	if(t->sue)
		return sue_align(t->sue, from);

	/* align to the size,
	 * except for double and ldouble
	 * (long changes but this is accounted for in type_primitive_size)
	 */
	switch(t->primitive){
		case type_double:
			if(IS_32_BIT()){
				/* 8 on Win32, 4 on Linux32 */
				if(platform_sys() == SYS_cygwin)
					return 8;
				return 4;
			}
			return 8; /* 8 on 64-bit */

		case type_ldouble:
			return IS_32_BIT() ? 4 : 16;

		default:
			return type_primitive_size(t->primitive);
	}
}

const char *btype_to_str(const btype *t)
{
#define BUF_SIZE (sizeof(buf) - (bufp - buf))
	static char buf[BTYPE_STATIC_BUFSIZ];
	char *bufp = buf;

	switch(t->primitive){
		case type_void:
		case type__Bool:
		case type_nchar: case type_schar: case type_uchar:
		case type_short: case type_ushort:
		case type_int:   case type_uint:
		case type_long:  case type_ulong:
		case type_float:
		case type_double:
		case type_llong: case type_ullong:
		case type_ldouble:
			snprintf(bufp, BUF_SIZE, "%s",
					type_primitive_to_str(t->primitive));
			break;

		case type_unknown:
			ICE("unknown type primitive");

		case type_enum:
		case type_struct:
		case type_union:
			snprintf(bufp, BUF_SIZE, "%s %s",
					sue_str(t->sue),
					t->sue->spel);
	}

	return buf;
}

unsigned type_primitive_size(enum type_primitive tp)
{
	switch(tp){
		case type__Bool:
		case type_void:
			return 1;

		case type_schar:
		case type_uchar:
		case type_nchar:
			return UCC_SZ_CHAR;

		case type_short:
		case type_ushort:
			return UCC_SZ_SHORT;

		case type_int:
		case type_uint:
			return UCC_SZ_INT;

		case type_float:
			return 4;

		case type_double:
			return 8;

		case type_long:
		case type_ulong:
			if(IS_32_BIT())
				return UCC_SZ_LONG_M32;
			return UCC_SZ_LONG_M64;

		case type_llong:
		case type_ullong:
			return UCC_SZ_LONG_LONG;

		case type_ldouble:
			/* 80-bit float */
			ICW("TODO: long double");
			return IS_32_BIT() ? 12 : 16;

		case type_enum:
		case type_union:
		case type_struct:
			ICE("s/u/e size");

		case type_unknown:
			break;
	}

	ICE("type %s in %s()",
			type_primitive_to_str(tp), __func__);
	return -1;
}

unsigned long long
type_primitive_max(enum type_primitive p, int is_signed)
{
	unsigned long long max;

	switch(p){
		case type__Bool: return 1;
		case type_nchar:
		case type_schar:
		case type_uchar:
			max = UCC_SCHAR_MAX;
			break;

		case type_short: max = UCC_SHRT_MAX; break;
		case type_int:   max = UCC_INT_MAX; break;
		case type_long:  max = IS_32_BIT() ? UCC_LONG_MAX_M32 : UCC_LONG_MAX_M64; break;
		case type_llong: max = UCC_LONG_LONG_MAX; break;

		case type_float:
		case type_double:
		case type_ldouble:
			/* 80-bit float */
			ICE("TODO: float max");

		case type_union:
		case type_struct:
		case type_enum:
			ICE("sue max");

		case type_void:
		case type_unknown:
		default:
			ICE("bad primitive %s", type_primitive_to_str(p));
	}

	return is_signed ? max : max * 2 + 1;
}

int type_floating(enum type_primitive p)
{
	switch(p){
		case type_float:
		case type_double:
		case type_ldouble:
			return 1;
		default:
			return 0;
	}
}

const char *type_primitive_to_str(const enum type_primitive p)
{
	switch(p){
		case type_nchar:  return "char";
		case type_schar:  return "signed char";
		case type_uchar:  return "unsigned char";

		CASE_STR_PREFIX(type, void);
		CASE_STR_PREFIX(type, short);
		CASE_STR_PREFIX(type, int);
		CASE_STR_PREFIX(type, long);
		case type_ushort: return "unsigned short";
		case type_uint:   return "unsigned int";
		case type_ulong:  return "unsigned long";
		CASE_STR_PREFIX(type, float);
		CASE_STR_PREFIX(type, double);
		CASE_STR_PREFIX(type, _Bool);

		case type_llong:   return "long long";
		case type_ullong:  return "unsigned long long";
		case type_ldouble: return "long double";

		CASE_STR_PREFIX(type, struct);
		CASE_STR_PREFIX(type, union);
		CASE_STR_PREFIX(type, enum);

		CASE_STR_PREFIX(type, unknown);
	}
	return NULL;
}

const char *type_cmp_to_str(enum type_cmp cmp)
{
	switch(cmp){
		CASE_STR_PREFIX(TYPE, EQUAL);
		CASE_STR_PREFIX(TYPE, EQUAL_TYPEDEF);
		CASE_STR_PREFIX(TYPE, QUAL_ADD);
		CASE_STR_PREFIX(TYPE, QUAL_SUB);
		CASE_STR_PREFIX(TYPE, QUAL_POINTED_ADD);
		CASE_STR_PREFIX(TYPE, QUAL_POINTED_SUB);
		CASE_STR_PREFIX(TYPE, QUAL_NESTED_CHANGE);
		CASE_STR_PREFIX(TYPE, CONVERTIBLE_IMPLICIT);
		CASE_STR_PREFIX(TYPE, CONVERTIBLE_EXPLICIT);
		CASE_STR_PREFIX(TYPE, NOT_EQUAL);
	}
	return NULL;
}

const char *type_qual_to_str(const enum type_qualifier qual, int trailing_space)
{
	static char buf[32];

	/* trailing space is purposeful */
	snprintf(buf, sizeof buf, "%s%s%s",
		qual & qual_const    ? "const "    : "",
		qual & qual_volatile ? "volatile " : "",
		qual & qual_restrict ? "restrict " : "");

	if(!trailing_space){
		char *last = strrchr(buf, ' ');
		if(last)
			*last = '\0';
	}

	return buf;
}
