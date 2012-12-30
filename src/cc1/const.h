#ifndef CONST_H
#define CONST_H

void const_fold(expr *e, consty *);
/*int const_expr_is_const(expr *e);*/
int const_expr_and_zero(expr *e);
/*long const_expr_value(expr *e);*/

#define const_fold_need_val(e, iv_addr) do{       \
		consty k;                                     \
		const_fold(e, &k);                            \
		UCC_ASSERT(k.type == CONST_WITH_VAL,          \
				"not const");                             \
		memcpy(iv_addr, &k.bits.iv, sizeof *iv_addr); \
	}while(0)

#define POSSIBLE_OPT(e, s) \
	cc1_warn_at(&e->where, 0, 1, WARN_OPT_POSSIBLE,  \
			"optimisation possible - %s (%s%s%s)",       \
			s,                                           \
			e->f_str(),                                  \
			expr_kind(e, op) ? " - " : "",               \
			expr_kind(e, op) ? op_to_str(e->op) : "")


#define OPT_CHECK(e, s)          \
	do{ consty k;                  \
		const_fold(e, &k);           \
		if(k.type == CONST_WITH_VAL) \
			POSSIBLE_OPT(e, s);        \
	}while(0)

#endif
