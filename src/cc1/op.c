#include <stdlib.h>
#include <assert.h>

#include "op.h"
#include "macros.h"

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

int op_can_float(enum op_type o)
{
	switch(o){
		case op_modulus:
		case op_xor:
		case op_or:
		case op_and:
		case op_shiftl:
		case op_shiftr:
		case op_bnot:
			return 0;
		default:
			return 1;
	}
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
			assert(0);
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

int op_returns_bool(enum op_type o)
{
	return o == op_not || op_is_comparison(o) || op_is_shortcircuit(o);
}
