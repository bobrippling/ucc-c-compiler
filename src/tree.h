#ifndef TREE_H
#define TREE_H

typedef struct expr expr;
typedef struct tree      tree;
typedef struct decl      decl;
typedef struct function  function;
typedef struct symtable  symtable;
typedef struct sym       sym;
typedef struct tree_flow tree_flow;
typedef struct type      type;

typedef struct where
{
	const char *fname;
	int line, chr;
} where;

enum type_primitive
{
#define type_ptr type_int
	type_int,
	type_char,
	type_void,
	type_unknown
};

enum type_spec
{
	spec_none = 0,
	spec_const  = 1 << 0,
	spec_extern = 1 << 1
};

struct type
{
	where where;

	enum type_primitive primitive;
	enum type_spec      spec;
	int func;
	int ptr_depth;
};

struct decl
{
	where where;

	type *type;

	char *spel;

	expr **arraysizes;
	/*
	 * int x[5][]; -> arraysizes = { expr(5), NULL }
	 *
	 * for the last [], ptr_depth is ++'d
	 */
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
		expr_str, /* "abc" */
		expr_identifier,
		expr_assign,
		expr_funcall,
		expr_cast
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

	expr *lhs, *rhs;

	int val;
	char *spel; /* variable name, OR string literal, in which case, val is the length */
	expr *expr; /* x = 5; expr is the 5 */
	expr **funcargs;

	type *vartype; /* type propagation */

	sym *sym; /* used for strings, points to the string's symtable entry */
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

	decl *func_decl;
	decl **args;
	tree *code;
	symtable *symtab;
};

tree      *tree_new();
expr      *expr_new();
type      *type_new();
decl      *decl_new();
function  *function_new();
type      *type_copy(type *);

tree_flow *tree_flow_new();

const char *op_to_str(  enum op_type   o);
const char *expr_to_str(enum expr_type t);
const char *stat_to_str(enum stat_type t);
const char *type_to_str(type          *t);
const char *spec_to_str(enum type_spec s);
const char *where_str(  struct where *w);

#define free_type(x) free(x)

#endif
