#ifndef __BUILTIN_H
#define __BUILTIN_H

#define BUILTIN_SPEL(e) (e)->bits.ident.spel

expr *builtin_new_reg_save_area(void);
expr *builtin_new_frame_address(int depth);

expr *builtin_parse(const char *sp);
expr *parse_any_args(void);

#define expr_mutate_builtin(exp, to)  \
	exp->f_fold = fold_ ## to

#define expr_mutate_builtin_gen(exp, to) \
	expr_mutate_builtin(exp, to),          \
	exp->f_gen        = gen_ ## to


#endif
