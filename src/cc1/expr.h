#ifndef EXPR_H
#define EXPR_H

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

	int freestanding; /* e.g. 1; needs use, whereas x(); doesn't - freestanding */


	enum op_type op;

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
	int in_parens; /* for if((x = 5)) testing */

	char *spel;
	expr *expr; /* x = 5; expr is the 5 */
	expr **funcargs;
	stmt *code; /* ({ ... }) */

	sym *sym;

	/* type propagation */
	decl *tree_type;
	array_decl *array_store;
};


expr *expr_new(            func_fold *f_fold, func_str *f_str, func_gen *f_gen, func_gen *f_gen_str);
void  expr_mutate(expr *e, func_fold *f_fold, func_str *f_str, func_gen *f_gen, func_gen *f_gen_str);

expr *expr_new_intval(intval *);

/* simple wrappers */
expr *expr_ptr_multiply(expr *, decl *);
expr *expr_assignment(expr *to, expr *from);
expr *expr_new_decl_init(decl *d);

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
#include "ops/expr_stmt.h"

#define expr_new_wrapper(type)       expr_new(      fold_expr_ ## type, str_expr_ ## type, gen_expr_ ## type, gen_expr_str_ ## type)
#define expr_mutate_wrapper(e, type) expr_mutate(e, fold_expr_ ## type, str_expr_ ## type, gen_expr_ ## type, gen_expr_str_ ## type)

#define expr_free(x) do{decl_free((x)->tree_type); free(x);}while(0)

#define expr_kind(exp, kind) ((exp)->f_fold == fold_expr_ ## kind)

#define expr_new_sizeof()       expr_new_wrapper(sizeof)

expr *expr_new_identifier(char *sp);
expr *expr_new_cast(decl *cast_to);
expr *expr_new_val(int val);
expr *expr_new_op(enum op_type o);
expr *expr_new_if(expr *test);
expr *expr_new_addr(void);
expr *expr_new_assign(void);
expr *expr_new_comma(void);
expr *expr_new_funcall(void);
expr *expr_new_stmt(stmt *code);

#endif
