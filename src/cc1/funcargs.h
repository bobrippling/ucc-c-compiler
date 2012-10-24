#ifndef FUNCARGS_H
#define FUNCARGS_H

enum funcargs_cmp
{
	funcargs_are_equal,
	funcargs_are_mismatch_types,
	funcargs_are_mismatch_count
};

/* if fspel ! NULL, print warnings */
enum funcargs_cmp funcargs_equal(funcargs *args_a, funcargs *args_b,
		int strict_types, const char *fspel);


funcargs *funcargs_new(void);
void funcargs_empty(funcargs *func);
void funcargs_free(funcargs *args, int free_decls);

#endif
