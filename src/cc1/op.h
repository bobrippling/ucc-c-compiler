#ifndef OP_H
#define OP_H

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

const char *op_to_str(enum op_type o);

int op_is_commutative(enum op_type o);
int op_is_shortcircuit(enum op_type o);
int op_is_comparison(enum op_type o);
int op_returns_bool(enum op_type o); /* comparison or short circuit */
int op_can_compound(enum op_type o);
int op_can_float(enum op_type o);

int op_increases(enum op_type);

#endif
