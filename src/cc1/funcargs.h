#ifndef FUNCARGS_H
#define FUNCARGS_H

enum funcargs_cmp
{
	FUNCARGS_ARE_EQUAL,
	FUNCARGS_ARE_MISMATCH_TYPES,
	FUNCARGS_ARE_MISMATCH_COUNT
};

struct funcargs
{
	where where;

	int args_void_implicit; /* f(){} - implicitly (void) */
	int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
	int args_old_proto; /* true if f(a, b); where a and b are identifiers */
	decl **arglist;
	int variadic;
	enum calling_conv conv;
};

/* if fspel ! NULL, print warnings */
enum funcargs_cmp funcargs_equal(funcargs *args_a, funcargs *args_b,
		int strict_types, const char *fspel);


funcargs *funcargs_new(void);
void funcargs_empty(funcargs *func);
void funcargs_free(funcargs *args, int free_decls, int free_refs);

#endif
