#ifndef OPS_BUILTIN_H
#define OPS_BUILTIN_H

#include "../expr.h"

#define BUILTIN_SPEL(e) (e)->bits.ident.bits.ident.spel

expr *builtin_new_reg_save_area(void);
expr *builtin_new_frame_address(int depth);

expr *builtin_parse(const char *sp, symtable *scope);
expr *parse_any_args(symtable *scope);

#define expr_mutate_builtin(exp, to)  \
	exp->f_fold = fold_ ## to,          \
	exp->f_gen = builtin_gen_ ## to

expr *builtin_new_memset(expr *p, int ch, size_t len);
expr *builtin_new_memcpy(expr *to, expr *from, size_t len);

#endif
