#ifndef CONST_H
#define CONST_H

void const_fold(expr *e, consty *);

int const_expr_and_zero(expr *e);
int const_expr_and_non_zero(expr *e);

void const_fold_integral(expr *e, numeric *);
integral_t const_fold_val_i(expr *e);

integral_t const_op_exec(
		/* rval is optional */
		integral_t lval, const integral_t *rval,
		enum op_type op, int is_signed,
		const char **error);

floating_t const_op_exec_fp(
		floating_t lv, const floating_t *rv,
		enum op_type op);

#endif
