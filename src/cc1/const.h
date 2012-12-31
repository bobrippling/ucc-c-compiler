#ifndef CONST_H
#define CONST_H

void const_fold(expr *e, consty *);
/*int const_expr_is_const(expr *e);*/
int const_expr_and_zero(expr *e);
/*long const_expr_value(expr *e);*/

#define const_fold_need_val(e, iv_addr) do{       \
		consty k;                                     \
		const_fold(e, &k);                            \
		UCC_ASSERT(k.type == CONST_VAL,               \
				"not const");                             \
		memcpy(iv_addr, &k.bits.iv, sizeof *iv_addr); \
	}while(0)

#endif
