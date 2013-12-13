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
		struct
		{
			decl_init **inits;
			struct init_cpy
			{
				decl_init *range_init;
				expr *first_instance;
			} **range_inits;

			/* see decl_init.doc */
		} ar;
		struct init_cpy **range_copy;
#define DECL_INIT_COPY_IDX_INITS(this, inits) \
		((this)->bits.range_copy - (inits))

#define DECL_INIT_COPY_IDX(this, array) \
		DECL_INIT_COPY_IDX_INITS(this,      \
				(array)->bits.ar.range_inits)

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
decl_init *decl_init_new_w(enum decl_init_type, where *);
const char *decl_init_to_str(enum decl_init_type);

/* returns 1 if const, 0 if non const.
 * if the init contains a non-standard constant expression (e.g. comma-expr),
 * *nonstd is set if nonstd isn't NULL
 */
int decl_init_is_const(
		decl_init *dinit, symtable *stab, expr **nonstd);

int decl_init_is_zero(decl_init *dinit);

void decl_init_brace_up_fold(decl *d, symtable *); /* normalises braces */

/* used for default initialising tenatives */
void decl_default_init(decl *d, symtable *stab);

/* creates assignment exprs - only used for local inits */
void decl_init_create_assignments_base(
		decl_init *init,
		type_ref *tfor, expr *base,
		stmt *code);

#endif
