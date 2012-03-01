#ifndef EXPR_H
#define EXPR_H

typedef struct expr expr;

typedef void         func_fold(     expr *, symtable *);
typedef void         func_gen(      expr *, symtable *);
typedef void         func_gen_store(expr *, symtable *);
typedef void         func_gen_1(    expr *, FILE *);
typedef int          func_const(    expr *);
typedef const char  *func_str(void);

struct expr
{
	where where;

	func_fold      *f_fold;
	func_const     *f_const_fold; /* optional */

	func_gen       *f_gen;
	func_gen_1     *f_gen_1; /* optional */
	func_gen_store *f_store; /* optional */

	func_str       *f_str;


	enum op_type
	{
		op_multiply,
		op_divide,
		op_plus,
		op_minus,
		op_modulus,
		op_deref,

		op_eq, op_ne,
		op_le, op_lt,
		op_ge, op_gt,

		op_xor,
		op_or,   op_and,
		op_orsc, op_andsc,
		op_not,  op_bnot,

		op_shiftl, op_shiftr,

		op_struct_ptr, op_struct_dot,

		op_unknown
	} op;

	int assign_is_post; /* do we return the altered value or the old one? */
#define expr_is_default assign_is_post
#define expr_is_sizeof  assign_is_post

	expr *lhs, *rhs;

	union
	{
		intval iv;
		char *s;
	} val;

	int ptr_safe; /* does val point to a string we know about? */

	char *spel;
	expr *expr; /* x = 5; expr is the 5 */
	expr **funcargs;

	sym *sym;

	/* type propagation */
	decl *tree_type;
	array_decl *array_store;
};


expr *expr_new(   func_fold *f_fold, func_gen *f_gen, func_str *f_str);
void  expr_mutate(expr *e, func_fold *f_fold, func_gen *f_gen, func_str *f_str);

expr *expr_new_intval(intval *);

expr *expr_ptr_multiply(expr *, decl *);
expr *expr_assignment(expr *to, expr *from);

#define expr_free(x) do{decl_free((x)->tree_type); free(x);}while(0)

#include "ops/expr_addr.h"
#include "ops/expr_assign.h"
#include "ops/expr_cast.h"
#include "ops/expr_comma.h"
#include "ops/expr_funcall.h"
#include "ops/expr_identifier.h"
#include "ops/expr_if.h"
#include "ops/expr_op.h"
#include "ops/expr_sizeof.h"
#include "ops/expr_val.h"

#define expr_new_wrapper(type)       expr_new(expr_fold_ ## type, expr_gen_ ## type, expr_str_ ## type)
#define expr_mutate_wrapper(e, type) expr_mutate(e, expr_fold_ ## type, expr_gen_ ## type, expr_str_ ## type)

#define expr_kind(exp, kind) ((exp)->f_fold == expr_fold_ ## kind)

#define expr_new_sizeof()       expr_new_wrapper(sizeof)
#define expr_new_funcall()      expr_new_wrapper(funcall)

expr *expr_new_identifier(char *sp);
expr *expr_new_cast(decl *cast_to);
expr *expr_new_val(int val);
expr *expr_new_op(enum op_type o);
expr *expr_new_if(expr *test);
expr *expr_new_addr(void);
expr *expr_new_assign(void);
expr *expr_new_comma(void);

#endif
