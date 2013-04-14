#ifndef DECL_INIT_H
#define DECL_INIT_H

typedef struct desig desig;

struct decl_init
{
	where where;

	enum decl_init_type
	{
		decl_init_scalar,  /* = [0-9] | basic-expr */
		decl_init_brace,   /* { `decl_init`, `decl_init`, ... } */
		decl_init_copy,    /* used in range init - references where we copy from */
	} type;

	union
	{
		expr *expr;
		decl_init **inits;
		size_t copy_idx;
	} bits;

	struct desig
	{
		enum { desig_ar, desig_struct } type;
		union
		{
			expr *range[2];
			char *member;
		} bits;
		struct desig *next; /* [0].a.b[1] */
	} *desig;
};
#define DESIG_TO_STR(t) ((t) == desig_ar ? "array" : "struct")

decl_init *decl_init_new(enum decl_init_type);
const char *decl_init_to_str(enum decl_init_type);
int         decl_init_is_const(decl_init *dinit, symtable *stab);
int         decl_init_is_zero(decl_init *dinit);

void decl_init_brace_up_fold(decl *d, symtable *); /* normalises braces */

/* creates assignment exprs - only used for local inits */
void decl_init_create_assignments_base(
		decl_init *init,
		type_ref *tfor, expr *base,
		stmt *code);

#endif
