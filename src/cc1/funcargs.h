#ifndef FUNCARGS_H
#define FUNCARGS_H

#include "decl.h"

enum funcargs_cmp
{
	FUNCARGS_EXACT_EQUAL,
	FUNCARGS_IMPLICIT_CONV,
	FUNCARGS_MISMATCH_TYPES,
	FUNCARGS_MISMATCH_COUNT
};

typedef struct funcargs funcargs;
struct funcargs
{
	where where;

	int args_void_implicit; /* f(){} - implicitly (void) */
	int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
	int args_old_proto; /* true if f(a, b); where a and b are identifiers */
	decl **arglist;
	int variadic;
	enum calling_conv conv;

	unsigned retains;
};

#define FUNCARGS_EMPTY_NOVOID(fa) \
		(!(fa)->arglist && !(fa)->args_void)

enum funcargs_cmp funcargs_cmp(funcargs *args_to, funcargs *args_from);


funcargs *funcargs_new(void);
void funcargs_empty(funcargs *func);
void funcargs_free(funcargs *args, int free_decls);

int funcargs_is_old_func(funcargs *);

void funcargs_ty_calc(funcargs *fa, unsigned *n_int, unsigned *n_fp);

#endif
