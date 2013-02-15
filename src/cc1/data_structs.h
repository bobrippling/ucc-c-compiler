typedef struct intval intval;
typedef struct stringval stringval;

struct intval
{
	long val;
	enum
	{
		VAL_UNSIGNED = 1 << 0,
		VAL_LONG     = 1 << 1
	} suffix;
};

struct stringval
{
	char *lbl;
	const char *str;
	unsigned len;
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
