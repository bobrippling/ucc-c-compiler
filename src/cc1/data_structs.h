#include <stdint.h>

typedef struct intval intval;
typedef struct type_ref  type_ref;

typedef unsigned long long intval_t;
typedef   signed long long sintval_t;
#define INTVAL_FMT_D "lld"
#define INTVAL_FMT_U "llu"
#define INTVAL_FMT_X "llx"
#define INTVAL_T_MAX ULLONG_MAX
struct intval
{
	intval_t val;
	enum intval_suffix
	{
		VAL_UNSIGNED = 1 << 0,
		VAL_LONG     = 1 << 1,
		VAL_LLONG    = 1 << 2,

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

#define INTVAL_BUF_SIZ 32
int intval_cmp(const intval *, const intval *);
int intval_str(char *buf, size_t nbuf, intval_t v, type_ref *ty);
intval_t intval_truncate(
		intval_t val, unsigned bytes, sintval_t *sign_extended);

intval_t intval_truncate_bits(
		intval_t val, unsigned bits,
		sintval_t *signed_iv);

int intval_high_bit(const intval_t val, type_ref *ty);

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
