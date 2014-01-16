#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include "../util/where.h"
#include "../util/util.h"

#include "data_structs.h"
#include "expr.h"
#include "sue.h"
#include "type.h"
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
