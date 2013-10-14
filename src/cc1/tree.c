#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "../util/alloc.h"
#include "../util/util.h"
#include "data_structs.h"
#include "macros.h"
#include "sym.h"
#include "../util/platform.h"
#include "sue.h"
#include "decl.h"
#include "cc1.h"
#include "defs.h"

const where *eof_where = NULL;

numeric *numeric_new(long v)
{
	numeric *num = umalloc(sizeof *num);
	num->val.i = v;
	return num;
}

int numeric_cmp(const numeric *a, const numeric *b)
{
	const integral_t la = a->val.i, lb = b->val.i;

	if(la > lb)
		return 1;
	if(la < lb)
		return -1;
	return 0;
}

int integral_str(char *buf, size_t nbuf, integral_t v, type_ref *ty)
{
	const int is_signed = type_ref_is_signed(ty);

	return snprintf(
			buf, nbuf,
			is_signed
				? "%" NUMERIC_FMT_D
				: "%" NUMERIC_FMT_U,
			v, is_signed);
}

integral_t integral_truncate_bits(
		integral_t val, unsigned bits,
		sintegral_t *signed_iv)
{
	integral_t pos_mask = ~(~0ULL << bits);
	integral_t truncated = val & pos_mask;

	if(signed_iv){
		sintegral_t sig = truncated;

		if((sintegral_t)val < 0){
			/* sign extend */
			unsigned revbits = sizeof(integral_t) * CHAR_BIT - bits;

			sig = (sig << revbits) >> revbits;
		}

		*signed_iv = sig;
	}

	return truncated;
}

integral_t integral_truncate(
		integral_t val, unsigned bytes,
		sintegral_t *sign_extended)
{
	switch(bytes){
#define CAST(sz)                                    \
		case sz:                                        \
			val = integral_truncate_bits(                 \
			    val, bytes * CHAR_BIT - 1, sign_extended);\
			break

		CAST(1);
		CAST(2);
		CAST(4);

#undef CAST

		case 8:
			if(sign_extended)
				*sign_extended = val;
			break; /* no cast - max word size */

		default:
			ICW("can't truncate numeric to %d bytes", bytes);
	}

	return val;
}

int integral_is_64_bit(const integral_t val, type_ref *ty)
{
	/* must apply the truncation here */
	integral_t trunc;

	if(type_ref_size(ty, &ty->where) < platform_word_size())
		return 0;

	trunc = integral_truncate(
			val, type_ref_size(ty, &ty->where), NULL);

#define INT_SHIFT (CHAR_BIT * sizeof(int))
	if(type_ref_is_signed(ty)){
		const sintegral_t as_signed = val;

		if(as_signed < 0){
			/* need unsigned (i.e. shr) shift */
			return (int)((integral_t)trunc >> INT_SHIFT) != -1;
		}
	}

	return (trunc >> INT_SHIFT) != 0;
#undef INT_SHIFT
}

static type *type_new_primitive1(enum type_primitive p)
{
	type *t = umalloc(sizeof *t);
	where_cc1_current(&t->where);
	t->primitive = p;
	return t;
}

const type *type_new_primitive_sue(enum type_primitive p, struct_union_enum_st *s)
{
	type *t = type_new_primitive1(p);
	t->sue = s;
	return t;
}

const type *type_new_primitive(enum type_primitive p)
{
	return type_new_primitive1(p);
}

unsigned type_primitive_size(enum type_primitive tp)
{
	switch(tp){
		case type_schar:
		case type_uchar:
		case type_nchar:

		case type__Bool:
		case type_void:
			return 1;

		case type_short:
		case type_ushort:
			return 2;

		case type_enum:
		case type_int:
		case type_uint:
		case type_float:
			return 4;

		case type_long:
		case type_ulong:
		case type_double:
			/* 4 on 32-bit */
			if(IS_32_BIT())
				return 4;
			/* fall */
		case type_llong:
		case type_ullong:
			return 8;

		case type_ldouble:
			/* 80-bit float */
			ICW("TODO: long double");
			return IS_32_BIT() ? 12 : 16;

		case type_union:
		case type_struct:
			ICE("s/u size");

		case type_unknown:
			break;
	}

	ICE("type %s in %s()",
			type_primitive_to_str(tp), __func__);
	return -1;
}

unsigned type_size(const type *t, where *from)
{
	if(t->sue)
		return sue_size(t->sue, from);

	return type_primitive_size(t->primitive);
}

unsigned type_align(const type *t, where *from)
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

const char *op_to_str(const enum op_type o)
{
	switch(o){
		case op_multiply: return "*";
		case op_divide:   return "/";
		case op_plus:     return "+";
		case op_minus:    return "-";
		case op_modulus:  return "%";
		case op_eq:       return "==";
		case op_ne:       return "!=";
		case op_le:       return "<=";
		case op_lt:       return "<";
		case op_ge:       return ">=";
		case op_gt:       return ">";
		case op_or:       return "|";
		case op_xor:      return "^";
		case op_and:      return "&";
		case op_orsc:     return "||";
		case op_andsc:    return "&&";
		case op_not:      return "!";
		case op_bnot:     return "~";
		case op_shiftl:   return "<<";
		case op_shiftr:   return ">>";
		CASE_STR_PREFIX(op, unknown);
	}
	return NULL;
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

const char *type_qual_to_str(const enum type_qualifier qual, int trailing_space)
{
	static char buf[32];
	/* trailing space is purposeful */
	snprintf(buf, sizeof buf, "%s%s%s%s",
		qual & qual_const    ? "const"    : "",
		qual & qual_volatile ? "volatile" : "",
		qual & qual_restrict ? "restrict" : "",
		qual && trailing_space ? " " : "");
	return buf;
}

int op_can_compound(enum op_type o)
{
	switch(o){
		case op_plus:
		case op_minus:
		case op_multiply:
		case op_divide:
		case op_modulus:
		case op_not:
		case op_bnot:
		case op_and:
		case op_or:
		case op_xor:
		case op_shiftl:
		case op_shiftr:
			return 1;
		default:
			break;
	}
	return 0;
}

int op_can_float(enum op_type o)
{
	switch(o){
		case op_modulus:
		case op_xor:
		case op_or:
		case op_and:
		case op_shiftl:
		case op_shiftr:
		case op_bnot:
			return 0;
		default:
			return 1;
	}
}

int op_is_commutative(enum op_type o)
{
	switch(o){
		case op_multiply:
		case op_plus:
		case op_xor:
		case op_or:
		case op_and:
		case op_eq:
		case op_ne:
			return 1;

		case op_unknown:
			ICE("bad op");
		case op_minus:
		case op_divide:
		case op_modulus:
		case op_orsc:
		case op_andsc:
		case op_shiftl:
		case op_shiftr:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
		case op_not:
		case op_bnot:
			break;
	}
	return 0;
}

int op_is_comparison(enum op_type o)
{
	switch(o){
		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
			return 1;
		default:
			break;
	}
	return 0;
}

int op_is_shortcircuit(enum op_type o)
{
	switch(o){
		case op_andsc:
		case op_orsc:
			return 1;
		default:
			return 0;
	}
}

int op_returns_bool(enum op_type o)
{
	return op_is_comparison(o) || op_is_shortcircuit(o);
}

const char *type_to_str(const type *t)
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
