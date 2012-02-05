#ifndef CONST_H
#define CONST_H

int const_fold(expr *e);
int const_expr_is_const(expr *e);
int const_expr_is_zero( expr *e);

#define POSSIBLE_OPT(e, s) \
	warn_at(&e->where, "optimisation possible - %s (%s)", s, expr_to_str(e->type))

#define OPT_CHECK(e, s) \
	if(const_expr_is_const(e)) \
		POSSIBLE_OPT(e, s)

#endif
