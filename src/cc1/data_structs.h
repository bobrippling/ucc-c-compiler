#include <stdint.h>

typedef struct numeric numeric;
typedef struct stringval stringval;
typedef struct type_ref  type_ref;

typedef unsigned long long integral_t;
typedef   signed long long sintegral_t;
typedef        long double floating_t;
#define NUMERIC_FMT_D "lld"
#define NUMERIC_FMT_U "llu"
#define NUMERIC_FMT_X "llx"
#define NUMERIC_FMT_LD "Lf"
#define NUMERIC_T_MAX ULLONG_MAX
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

		/* variable was read in as:
		 * (decimal if neither of these set)
		 */
		VAL_OCTAL       = 1 << 3,
		VAL_HEX         = 1 << 4,
		VAL_BIN         = 1 << 5,
		VAL_NON_DECIMAL = VAL_OCTAL | VAL_HEX | VAL_BIN,
		VAL_PREFIX_MASK = VAL_NON_DECIMAL,
	} suffix;
};

int numeric_cmp(const numeric *, const numeric *);

#define INTEGRAL_BUF_SIZ 32
int integral_str(char *buf, size_t nbuf, integral_t v, type_ref *ty);

int integral_is_64_bit(const integral_t val, type_ref *ty);
integral_t integral_truncate(
		integral_t val, unsigned bytes, sintegral_t *sign_extended);

integral_t integral_truncate_bits(
		integral_t val,
		unsigned bits,
		sintegral_t *signed_v);

struct stringval
{
	where where;
	char *lbl;
	const char *str;
	unsigned len;
	int wide;
};

enum op_type
{
	/* binary */
	op_multiply, op_divide, op_modulus,
	op_plus, op_minus,
	op_xor, op_or, op_and,
	op_orsc, op_andsc,
	op_shiftl, op_shiftr,

	/* unary */
	op_not,  op_bnot,

	/* comparison */
	op_eq, op_ne,
	op_le, op_lt,
	op_ge, op_gt,

	op_unknown
};

#include "tree.h"
#include "expr.h"
#include "stmt.h"
#include "sym.h"
#include "decl.h"
