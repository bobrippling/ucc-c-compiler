#ifndef DECL_INIT_H
#define DECL_INIT_H

struct decl_init
{
	where where;

	enum decl_init_type /* TODO: ops/init_... */
	{
		/*decl_init_str - covered by scalar */
		decl_init_scalar,              /* = [0-9] | basic-expr */
		decl_init_brace,               /* { `decl_init`, `decl_init`, ... } */
		/*decl_init_struct,             * { .member1 = `decl_init`, .member2 = `decl_init` } */
	} type;

	union
	{
		expr *expr;
		decl_init **inits;
	} bits;
};

decl_init *decl_init_new(enum decl_init_type);
int        decl_init_len(decl_init *);
const char *decl_init_to_str(enum decl_init_type);
int         decl_init_is_const(decl_init *dinit, symtable *stab);
int         decl_init_is_zero(decl_init *dinit);

void decl_init_create_assignments_for_spel(decl *d, stmt *init_code);
void decl_init_create_assignments_for_base(decl *d, expr *base, stmt *init_code);

#endif
