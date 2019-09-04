#ifndef NUM_H
#define NUM_H

#include <stdint.h>
#include <limits.h>

#include "../util/compiler.h"

typedef struct numeric numeric;

typedef unsigned long long integral_t;
typedef signed long long sintegral_t;

#if COMPILER_SUPPORTS_LONG_DOUBLE
typedef long double floating_t;
#  define ucc_strtold strtold
#  define NUMERIC_FMT_LD "Lf"
#else
typedef double floating_t;
#  define ucc_strtold strtof
#  define NUMERIC_FMT_LD "f"
#endif

#define NUMERIC_FMT_D "lld"
#define NUMERIC_FMT_U "llu"
#define NUMERIC_FMT_X "llx"
#define NUMERIC_T_MAX ULLONG_MAX
#define INTEGRAL_BITS (sizeof(integral_t) * CHAR_BIT)
struct numeric
{
	union
	{
		integral_t i;
		floating_t   f;
	} val;
	enum numeric_suffix
	{
		VAL_UNSIGNED = 1 << 0,
		VAL_LONG     = 1 << 1,
		VAL_LLONG    = 1 << 2,

		VAL_FLOAT    = 1 << 6,
		VAL_DOUBLE   = 1 << 7,
		VAL_LDOUBLE  = 1 << 8,
		VAL_FLOATING = VAL_FLOAT | VAL_DOUBLE | VAL_LDOUBLE,

		/* variable was read in as decimal if none of these set */
		VAL_OCTAL       = 1 << 3,
		VAL_HEX         = 1 << 4,
		VAL_BIN         = 1 << 5,
		VAL_NON_DECIMAL = VAL_OCTAL | VAL_HEX | VAL_BIN,
		VAL_SUFFIXED_MASK = VAL_UNSIGNED | VAL_LONG | VAL_LLONG
	} suffix;
};

int numeric_cmp(const numeric *, const numeric *);

#define INTEGRAL_BUF_SIZ 32
struct type;
int integral_str(char *buf, size_t nbuf, integral_t v, struct type *ty);

integral_t integral_truncate(
		integral_t val, unsigned bytes, sintegral_t *sign_extended);

integral_t integral_truncate_bits(
		integral_t val,
		unsigned bits,
		sintegral_t *signed_v);

int integral_high_bit(const integral_t val, struct type *ty);

#endif
