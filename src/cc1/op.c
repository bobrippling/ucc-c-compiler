
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



op *op_new_divide    (expr *l, expr *r);
op *op_new_eq        (expr *l, expr *r);
op *op_new_ge        (expr *l, expr *r);
op *op_new_le        (expr *l, expr *r);
op *op_new_minus     (expr *l, expr *r);
op *op_new_modulus   (expr *l, expr *r);
op *op_new_or        (expr *l, expr *r);
op *op_new_orsc      (expr *l, expr *r);
op *op_new_plus      (expr *l, expr *r);
op *op_new_shiftl    (expr *l, expr *r);
op *op_new_struct_ptr(expr *l, expr *r);
op *op_new_xor       (expr *l, expr *r);
op *op_new_and       (expr *l, expr *r);
op *op_new_andsc     (expr *l, expr *r);
op *op_new_gt        (expr *l, expr *r);
op *op_new_lt        (expr *l, expr *r);
op *op_new_ne        (expr *l, expr *r);
op *op_new_shiftr    (expr *l, expr *r);
op *op_new_struct_dot(expr *l, expr *r);

op *op_from_token(enum token tok, expr *lhs, expr *rhs)
{
	switch(tok){
		case token_multiply:  return op_multiply;
		case token_divide:    return op_divide;
		case token_plus:      return op_plus;
		case token_minus:     return op_minus;
		case token_modulus:   return op_modulus;
		case token_eq:        return op_eq;
		case token_ne:        return op_ne;
		case token_le:        return op_le;
		case token_lt:        return op_lt;
		case token_ge:        return op_ge;
		case token_gt:        return op_gt;
		case token_xor:       return op_xor;
		case token_or:        return op_or;
		case token_and:       return op_and;
		case token_orsc:      return op_orsc;
		case token_andsc:     return op_andsc;
		case token_not:       return op_not;
		case token_bnot:      return op_bnot;
		case token_shiftl:    return op_shiftl;
		case token_shiftr:    return op_shiftr;
		case token_ptr:       return op_struct_ptr;
		case token_dot:       return op_struct_dot;
	}
}
