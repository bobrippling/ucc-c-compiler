#ifndef OP_H
#define OP_H

int op_is_cmp(enum op_type o)
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

typedef int         op_exec(expr *lhs, expr *rhs, enum op_type op, int *bad);
typedef int         op_optimise(expr *lhs, expr *rhs);

struct expr_op
{
	op_exec     *f_exec;
	func_str    *f_str;
	op_optimise *f_optimise; /* optional */
	func_fold   *f_fold;
	func_gen    *f_gen_str, *f_gen;
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

#endif
