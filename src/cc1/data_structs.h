typedef struct intval intval;
typedef struct stringval stringval;

int intval_cmp(const intval *, const intval *);

struct intval
{
	long val;
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

struct stringval
{
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
