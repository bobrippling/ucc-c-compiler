#ifndef TREE_H
#define TREE_H

typedef struct sym         sym;
typedef struct symtable    symtable;

typedef struct expr        expr;
typedef struct tree        tree;
typedef struct decl        decl;
typedef struct function    function;
typedef struct tree_flow   tree_flow;
typedef struct type        type;
typedef struct assignment  assignment;

enum type_primitive
{
	type_int,
	type_char,
	type_void,
	type_unknown
};

enum type_spec
{
	spec_none   = 0,
	spec_const  = 1 << 0,
	spec_extern = 1 << 1,
	spec_static = 1 << 2
};

struct type
{
	where where;

	enum type_primitive primitive;
	enum type_spec      spec;
};

struct decl
{
	where where;

	type *type;

	char *spel;

	expr **arraysizes;
	expr *init; /* NULL except for global variables */

	int ptr_depth;
	/*
	 * int x[5][]; -> arraysizes = { expr(5), NULL }
	 *
	 * for the last [], ptr_depth is ++'d
	 */

	function *func;

	int ignore; /* ignore during code-gen, for example ignoring overridden externs */
	sym *sym;
};

struct expr
{
	where where;

	enum expr_type
	{
		expr_op,
		expr_val,
		expr_addr, /* &x */
		expr_sizeof,
		expr_array, /* "abc" or { 1, 2, 3 } */
		expr_identifier,
		expr_assign,
		expr_funcall,
		expr_cast,
		expr_if,
		expr_comma
	} type;

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

		op_or,   op_and,
		op_orsc, op_andsc,
		op_not,  op_bnot,

		op_unknown
	} op;

	int assign_is_post; /* do we return the altered value or the old one? */

	expr *lhs, *rhs;

	union
	{
		int i;
		char *s;
		expr **exprs;
	} val; /*
				  * stores both int values,
					* and string pointers (used in const.c arithmetic)
					* and array members
					*/
	int arrayl; /* string or array length */
	int ptr_safe; /* does val point to a string we know about? */
	enum
	{
		ARRAY_STR,
		ARRAY_EXPR
	} array_type;

	char *spel;
	expr *expr; /* x = 5; expr is the 5 */
	expr **funcargs;

	sym *sym;

	/* type propagation */
	decl *tree_type;
};

struct tree
{
	where where;

	enum stat_type
	{
		stat_do,
		stat_if,
		stat_while,
		stat_for,
		stat_break,
		stat_return,
		stat_goto,
		stat_expr,
		stat_code,
		stat_noop
	} type;

	tree *lhs, *rhs;
	expr *expr; /* test expr for if and do, etc */

	tree_flow *flow; /* for, switch (do and while are simple enough for ->[lr]hs) */

	/* specific data */
	int val;
	function *func;
	decl **decls; /* block definitions, e.g. { int i... } */
	tree **codes; /* for a code block */

	symtable *symtab; /* pointer to the containing function's symtab */
};

struct tree_flow
{
	expr *for_init, *for_while, *for_inc;
	/* TODO: switch */
};

struct function
{
	where where;

	int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
	decl **args;
	symtable *autos;
	tree *code;
	int variadic;
};

tree      *tree_new();
expr      *expr_new();
type      *type_new();
decl      *decl_new();
decl      *decl_new_where(where *);
function  *function_new();

type      *type_copy(type *);
decl      *decl_copy(decl *);
/*expr      *expr_copy(expr *);*/
tree      *tree_new_code();
expr      *expr_new_val(int);

tree_flow *tree_flow_new();

const char *op_to_str(  const enum op_type   o);
const char *expr_to_str(const enum expr_type t);
const char *stat_to_str(const enum stat_type t);
const char *decl_to_str(const decl          *t);
const char *type_to_str(const type          *t);
const char *spec_to_str(const enum type_spec s);

int   type_equal(const type *, const type *);
int   decl_size(const decl *);
int   decl_equal(const decl *, const decl *, int strict);
expr *expr_ptr_multiply(expr *, decl *);
expr *expr_assignment(expr *to, expr *from);

#define DECL_STATIC_BUFSIZ 32

#define type_free(x) free(x)
#define decl_free(x) do{type_free((x)->type); free(x);}while(0)

#endif
