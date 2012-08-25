#ifndef EXPR_H
#define EXPR_H

enum constyness
{
	CONST_NO          = 0, /* f() */
	CONST_WITH_VAL,        /* 5 + 2 */
	CONST_WITHOUT_VAL,     /* &f where f is global */
};

typedef void         func_fold(     expr *, symtable *);
typedef void         func_gen(      expr *, symtable *);
typedef void         func_gen_store(expr *, symtable *);
typedef void         func_gen_1(    expr *, FILE *);
typedef void         func_const(    expr *, intval *val, enum constyness *success);
typedef const char  *func_str(void);
typedef void         func_mutate_expr(expr *);

struct expr
{
	where where;

	func_fold      *f_fold;
	func_const     *f_const_fold; /* optional */

	func_gen       *f_gen;
	func_gen_1     *f_gen_1; /* optional */
	func_gen_store *f_store; /* optional */

	func_str       *f_str;

	int freestanding; /* e.g. 1; needs use, whereas x(); doesn't - freestanding */


	enum op_type op;

	int assign_is_post; /* do we return the altered value or the old one? */
#define expr_is_default    assign_is_post
#define expr_computed_goto assign_is_post
#define expr_cast_implicit assign_is_post
#define expr_is_typeof     assign_is_post

	expr *lhs, *rhs;

	union
	{
		intval iv;
		char *s;
	} val;

	struct generic_lbl
	{
		decl *d; /* NULL -> default */
		expr *e;
	} **generics, *generic_chosen;

	int ptr_safe; /* does val point to a string we know about? */
	int in_parens; /* for if((x = 5)) testing */
	int op_no_ptr_mul; /* for &(a.b) -> (&a) + offsetof(a, b) - don't multiply the op */

	char *spel;
	expr *expr; /* x = 5; expr is the 5 */
	expr **funcargs;
	stmt *code; /* ({ ... }) */
	decl *decl; /* for sizeof(decl) */

	funcargs *block_args;

	sym *sym;

	/* type propagation */
	decl *tree_type;
	array_decl *array_store;
};


expr *expr_new(          func_mutate_expr *, func_fold *, func_str *, func_gen *, func_gen *, func_gen *);
void expr_mutate(expr *, func_mutate_expr *, func_fold *, func_str *, func_gen *, func_gen *, func_gen *);

#define expr_mutate_wrapper(e, type) expr_mutate(e,               \
                                        mutate_expr_     ## type, \
                                        fold_expr_       ## type, \
                                        str_expr_        ## type, \
                                        gen_expr_        ## type, \
                                        gen_expr_str_    ## type, \
                                        gen_expr_style_  ## type)

#define expr_new_wrapper(type) expr_new(mutate_expr_     ## type, \
                                        fold_expr_       ## type, \
                                        str_expr_        ## type, \
                                        gen_expr_        ## type, \
                                        gen_expr_str_    ## type, \
                                        gen_expr_style_  ## type)

expr *expr_new_intval(intval *);

/* simple wrappers */
expr *expr_new_decl_init(decl *d);
expr *expr_new_array_decl_init(decl *d, int ival, int idx);

#include "ops/expr_addr.h"
#include "ops/expr_assign.h"
#include "ops/expr_assign_compound.h"
#include "ops/expr_cast.h"
#include "ops/expr_comma.h"
#include "ops/expr_funcall.h"
#include "ops/expr_identifier.h"
#include "ops/expr_if.h"
#include "ops/expr_op.h"
#include "ops/expr_sizeof.h"
#include "ops/expr_val.h"
#include "ops/expr_stmt.h"
#include "ops/expr_deref.h"

#define expr_free(x) do{if((x)->tree_type) decl_free((x)->tree_type); free(x);}while(0)

#define expr_kind(exp, kind) ((exp)->f_fold == fold_expr_ ## kind)

expr *expr_new_identifier(char *sp);
expr *expr_new_cast(decl *cast_to);
expr *expr_new_val(int val);
expr *expr_new_op(enum op_type o);
expr *expr_new_if(expr *test);
expr *expr_new_stmt(stmt *code);
expr *expr_new_sizeof_decl(decl *, int is_typeof);
expr *expr_new_sizeof_expr(expr *, int is_typeof);
expr *expr_new_funcall(void);
expr *expr_new_assign(         expr *to, expr *from);
expr *expr_new_assign_compound(expr *to, expr *from, enum op_type);
expr *expr_new__Generic(expr *test, struct generic_lbl **lbls);
expr *expr_new_block(decl *rt, funcargs *args, stmt *code);
expr *expr_new_deref(expr *);

#define expr_new_addr()    expr_new_wrapper(addr)
#define expr_new_comma()   expr_new_wrapper(comma)

#endif
