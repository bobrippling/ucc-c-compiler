#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "../util/where.h"
#include "../util/util.h"
#include "../util/platform.h"

#include "data_structs.h"
#include "expr.h"
#include "sue.h"
#include "btype.h"
#include "cc1.h"

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

int type_primitive_is_signed(enum type_primitive p)
{
	switch(p){
		case type_nchar:
			/* XXX: note we treat char as signed */
			/* -fsigned-char */
			return !!(fopt_mode & FOPT_SIGNED_CHAR);

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
			ICE("%s(%s)",
					__func__,
					type_primitive_to_str(p));

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
	return type_primitive_is_signed(t->primitive);
}

int type_qual_loss(enum type_qualifier a, enum type_qualifier b)
{
	a &= ~qual_restrict,
	b &= ~qual_restrict;

	/* if we have b, remove all of a's bits and still have stuff,
	 * e.g. const or volatile, then a takes away b's qualifiers
	 */
	return b & ~a ? 1 : 0;
}

unsigned btype_size(const btype *t, where *from)
{
	if(t->sue)
		return sue_size(t->sue, from);

	return type_primitive_size(t->primitive);
}

unsigned btype_align(const btype *t, where *from)
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
				if(platform_sys() == PLATFORM_CYGWIN)
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
	static char buf[TYPE_STATIC_BUFSIZ];
	char *bufp = buf;

	if(t->sue){
		snprintf(bufp, BUF_SIZE, "%s %s",
				sue_str(t->sue),
				t->sue->spel);

	}else{
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
				ICE("unknown type primitive (%s)", where_str(&t->where));
			case type_enum:
			case type_struct:
			case type_union:
				ICE("struct/union/enum without ->sue");
		}
	}

	return buf;
}
