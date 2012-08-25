typedef struct intval intval;

struct intval
{
	long val;
	enum
	{
		VAL_UNSIGNED = 1 << 0,
		VAL_LONG     = 1 << 1
	} suffix;
};

enum op_type
{
	/* maths */
	op_multiply,
	op_divide,
	op_plus,
	op_minus,
	op_modulus,

	/* binary */
	op_xor,
	op_or,   op_and,
	op_orsc, op_andsc,
	op_not,  op_bnot,

	/* shift */
	op_shiftl, op_shiftr,

	/* comparison */
	op_eq, op_ne,
	op_le, op_lt,
	op_ge, op_gt,

	op_struct_ptr, op_struct_dot,

	op_unknown
};

#include "tree.h"
#include "expr.h"
#include "stmt.h"
#include "sym.h"
#include "decl.h"
