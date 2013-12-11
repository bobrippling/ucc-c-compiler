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
#include "../util/limits.h"
#include "sue.h"
#include "decl.h"
#include "cc1.h"
#include "defs.h"

const where *eof_where = NULL;

intval *intval_new(long v)
{
	intval *iv = umalloc(sizeof *iv);
	iv->val = v;
	return iv;
}

int intval_cmp(const intval *a, const intval *b)
{
	const intval_t la = a->val, lb = b->val;

	if(la > lb)
		return 1;
	if(la < lb)
		return -1;
	return 0;
}

int intval_str(char *buf, size_t nbuf, intval_t v, type_ref *ty)
{
	/* the final resting place for an intval */
	const int is_signed = type_ref_is_signed(ty);

	if(ty){
		sintval_t sv;
		v = intval_truncate(v, type_ref_size(ty, NULL), &sv);
		if(is_signed)
			v = sv;
	}

	return snprintf(
			buf, nbuf,
			is_signed
				? "%" INTVAL_FMT_D
				: "%" INTVAL_FMT_U,
			v, is_signed);
}

intval_t intval_truncate_bits(
		intval_t val, unsigned bits,
		sintval_t *signed_iv)
{
	intval_t pos_mask = ~(~0ULL << bits);
	intval_t truncated = val & pos_mask;

	if(fopt_mode & FOPT_CAST_W_BUILTIN_TYPES){
		/* we use sizeof our types so our conversions match the target conversions */
#define BUILTIN(type)                    \
		if(bits == sizeof(type) * CHAR_BIT){ \
			if(signed_iv)                      \
				*signed_iv = (signed type)val;   \
			return (unsigned type)val;         \
		}

		BUILTIN(char);
		BUILTIN(short);
		BUILTIN(int);
		BUILTIN(long);
		BUILTIN(long long);
	}

	/* not builtin - bitfield, etc */
	if(signed_iv){
		const unsigned shamt = CHAR_BIT * sizeof(val) - bits;

		/* implementation specific signed right shift.
		 * this is to sign extend the value
		 */
		*signed_iv = (sintval_t)val << shamt >> shamt;
	}

	return truncated;
}

intval_t intval_truncate(
		intval_t val, unsigned bytes,
		sintval_t *sign_extended)
{
	return intval_truncate_bits(
			val, bytes * CHAR_BIT,
			sign_extended);
}

int intval_high_bit(intval_t val, type_ref *ty)
{
	if(type_ref_is_signed(ty)){
		const sintval_t as_signed = val;

		if(as_signed < 0)
			val = intval_truncate(val, type_ref_size(ty, &ty->where), NULL);
	}

	{
		int r;
		for(r = -1; val; r++, val >>= 1);
		return r;
	}
}

static type *type_new_primitive1(enum type_primitive p)
{
	type *t = umalloc(sizeof *t);
	where_cc1_current(&t->where);
	t->primitive = p;
	t->is_signed = p != type__Bool;
	return t;
}

const type *type_new_primitive_sue(enum type_primitive p, struct_union_enum_st *s)
{
	type *t = type_new_primitive1(p);
	t->sue = s;
	return t;
}

const type *type_new_primitive_signed(enum type_primitive p, int sig)
{
	type *t = type_new_primitive1(p);
	t->is_signed = sig && p != type__Bool;
	return t;
}

const type *type_new_primitive(enum type_primitive p)
{
	return type_new_primitive1(p);
}

unsigned type_primitive_size(enum type_primitive tp)
{
	switch(tp){
		case type__Bool:
		case type_void:
			return 1;
		case type_char:
			return UCC_SZ_CHAR;

		case type_short:
			return UCC_SZ_SHORT;

		case type_int:
			return UCC_SZ_INT;
		case type_float:
			return 4;

		case type_long:
		case type_double:
			/* 4 on 32-bit */
			if(cc1_m32)
				return 4; /* FIXME: 32-bit long */
			return UCC_SZ_LONG;

		case type_llong:
			return UCC_SZ_LONG_LONG;

		case type_ldouble:
			/* 80-bit float */
			ICW("TODO: long double");
			return cc1_m32 ? 12 : 16;

		case type_union:
		case type_struct:
		case type_enum:
			ICE("sue size");

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
		case type_char:  max = UCC_SCHAR_MAX;     break;
		case type_short: max = UCC_SHRT_MAX;      break;
		case type_int:   max = UCC_INT_MAX;       break;
		case type_long:  max = UCC_LONG_MAX;      break;
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
			if(cc1_m32){
				/* 8 on Win32, 4 on Linux32 */
				if(platform_sys() == PLATFORM_CYGWIN)
					return 8;
				return 4;
			}
			return 8; /* 8 on 64-bit */

		case type_ldouble:
			return cc1_m32 ? 4 : 16;

		default:
			return type_primitive_size(t->primitive);
	}
}

int type_qual_equal(enum type_qualifier a, enum type_qualifier b)
{
 return (a | qual_restrict) == (b | qual_restrict);
}

int type_equal(const type *a, const type *b, enum type_cmp mode)
{
	if((mode & TYPE_CMP_ALLOW_SIGNED_UNSIGNED) == 0
	&& a->is_signed != b->is_signed)
	{
		return 0;
	}

	if(a->sue != b->sue)
		return 0;

	return mode & TYPE_CMP_EXACT ? a->primitive == b->primitive : 1;
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
		CASE_STR_PREFIX(type, void);
		CASE_STR_PREFIX(type, char);
		CASE_STR_PREFIX(type, short);
		CASE_STR_PREFIX(type, int);
		CASE_STR_PREFIX(type, long);
		CASE_STR_PREFIX(type, float);
		CASE_STR_PREFIX(type, double);
		CASE_STR_PREFIX(type, _Bool);

		case type_llong:   return "long long";
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

int op_is_relational(enum op_type o)
{
	return op_is_comparison(o) || op_is_shortcircuit(o);
}

const char *type_to_str(const type *t)
{
#define BUF_SIZE (sizeof(buf) - (bufp - buf))
	static char buf[TYPE_STATIC_BUFSIZ];
	char *bufp = buf;

	if(!t->is_signed && t->primitive != type__Bool)
		bufp += snprintf(bufp, BUF_SIZE, "unsigned ");

	if(t->sue){
		snprintf(bufp, BUF_SIZE, "%s %s",
				sue_str(t->sue),
				t->sue->spel);

	}else{
		switch(t->primitive){
			case type_void:
			case type__Bool:
			case type_char:
			case type_short:
			case type_int:
			case type_long:
			case type_float:
			case type_double:
			case type_llong:
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
