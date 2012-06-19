#ifndef CONST_H
#define CONST_H

int const_fold(expr *e);
int const_expr_is_const(expr *e);
int const_expr_is_zero( expr *e);
int const_expr_value(expr *e);

#define POSSIBLE_OPT(e, s) \
	cc1_warn_at(&e->where, 0, 1, WARN_OPT_POSSIBLE,  \
			"optimisation possible - %s (%s%s%s)",       \
			s,                                           \
			e->f_str(),                                  \
			expr_kind(e, op) ? " - " : "",               \
			expr_kind(e, op) ? op_to_str(e->op) : "")


#define OPT_CHECK(e, s)      \
	if(const_expr_is_const(e)) \
		POSSIBLE_OPT(e, s)

#endif
