#ifndef OPS_BUILTIN_H
#define OPS_BUILTIN_H

#include "../expr.h"

#define BUILTIN_SPEL(e) (e)->bits.ident.bits.ident.spel

expr *builtin_new_reg_save_area(void);
expr *builtin_new_frame_address(int depth);

expr *builtin_parse(const char *sp, symtable *scope);
expr *parse_any_args(symtable *scope);

const out_val *builtin_gen_print(const expr *e, out_ctx *octx);
#define BUILTIN_SET_GEN(exp, target)      \
	exp->f_gen = cc1_backend == BACKEND_ASM \
		? (target)                            \
		: builtin_gen_print

#define expr_mutate_builtin(exp, to)  \
	exp->f_fold = fold_ ## to

#define expr_mutate_builtin_gen(exp, to) \
	expr_mutate_builtin(exp, to),          \
	BUILTIN_SET_GEN(exp, builtin_gen_ ## to)


expr *builtin_new_memset(expr *p, int ch, size_t len);
expr *builtin_new_memcpy(expr *to, expr *from, size_t len);

#endif
