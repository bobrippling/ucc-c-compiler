#ifndef TREE_H
#define TREE_H

typedef struct where
{
	int line, chr;
} where;

typedef struct expr expr;
struct expr
{
	enum expr_type
	{
		expr_op,
		expr_val,
		expr_addr, /* &x */
		expr_sizeof,
		expr_str, /* "abc" */
		expr_identifier
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
	char *spel;
	where where;
};

typedef struct tree     tree;
typedef struct decl     decl;
typedef struct function function;

struct tree
{
	enum stat_type
	{
		stat_do,
		stat_if,
		stat_else,
		stat_while,
		stat_for,
		stat_break,
		stat_return,
		stat_expr,
		stat_assign,
		stat_funcall,
		stat_code,
		stat_noop
	} type;

	tree *lhs, *rhs;
	expr *expr; /* test expr for if and do, etc */
	where where;

	/* specific data */
	int val;
	function *func;
	decl **decls; /* block definitions, e.g. { int i... } */
	expr **funcargs;
	tree **codes; /* for a code block */
};

enum type
{
	type_int,
	type_byte,
	type_void,
	type_unknown
};

struct decl
{
	enum type type;
	int ptr_depth;
	char *spel;
};

struct function
{
	decl *func_decl;
	decl **args;
	tree *code;
};

tree *tree_new();
expr *expr_new();

#endif
