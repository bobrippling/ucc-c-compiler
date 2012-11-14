typedef struct intval intval;

typedef int (*intval_cmp_cast)(const void *, const void *);

int intval_cmp(const intval *, const intval *);

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
	op_multiply,
	op_divide,
	op_plus,
	op_minus,
	op_modulus,
	op_deref,

	op_eq, op_ne,
	op_le, op_lt,
	op_ge, op_gt,

	op_xor,
	op_or,   op_and,
	op_orsc, op_andsc,
	op_not,  op_bnot,

	op_shiftl, op_shiftr,

	op_struct_ptr, op_struct_dot,

	op_unknown
};

#include "tree.h"
#include "expr.h"
#include "stmt.h"
#include "sym.h"
#include "decl.h"
