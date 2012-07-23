#ifndef CONST_H
#define CONST_H

void const_fold(expr *e, intval *iv, enum constyness *success);
/*int const_expr_is_const(expr *e);*/
int const_expr_and_zero(expr *e);
/*long const_expr_value(expr *e);*/

#define POSSIBLE_OPT(e, s) \
	cc1_warn_at(&e->where, 0, 1, WARN_OPT_POSSIBLE,  \
			"optimisation possible - %s (%s%s%s)",       \
			s,                                           \
			e->f_str(),                                  \
			expr_kind(e, op) ? " - " : "",               \
			expr_kind(e, op) ? op_to_str(e->op) : "")


#define OPT_CHECK(e, s)                \
	do{ intval dummy; enum constyness d; \
		const_fold(e, &dummy, &d);         \
		if(d == CONST_WITH_VAL)            \
			POSSIBLE_OPT(e, s);              \
	}while(0)

#endif
