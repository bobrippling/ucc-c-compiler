#ifndef TREE_H
#define TREE_H

typedef struct sym         sym;
typedef struct symtable    symtable;

typedef struct tdef        tdef;
typedef struct struc       struc;

typedef struct expr        expr;
typedef struct tree        tree;
typedef struct decl        decl;
typedef struct array_decl  array_decl;
typedef struct function    function;
typedef struct tree_flow   tree_flow;
typedef struct type        type;
typedef struct assignment  assignment;
typedef struct label       label;

enum type_primitive
{
	type_int,
	type_char,
	type_void,

	type_typedef,
	type_struct,

	type_unknown
};

enum type_spec
{
	spec_none     = 0,
	spec_const    = 1 << 0,
	spec_extern   = 1 << 1,
	spec_static   = 1 << 2,
	spec_signed   = 1 << 3,
	spec_unsigned = 1 << 4,
	spec_auto     = 1 << 5,
	spec_typedef  = 1 << 6,
#define SPEC_MAX 7
};

struct type
{
	where where;

	enum type_primitive primitive;
	enum type_spec      spec;

	struc *struc; /* NULL unless this is a structure */
	decl  *tdef;
};

struct decl
{
	where where;

	int field_width;
	type *type;

	char *spel;

	expr **arraysizes;

	expr *init; /* NULL except for global variables */
	array_decl *arrayinit;

	int ptr_depth;
	/*
	 * int x[5][]; -> arraysizes = { expr(5), NULL }
	 *
	 * for the last [], ptr_depth is ++'d
	 */

	function *func;

	int ignore; /* ignore during code-gen, for example ignoring overridden externs */
#define struct_offset ignore
	sym *sym;
};

struct array_decl
{
	union
	{
		char *str;
		expr **exprs;
	} data;

	char *label;
	int len;

	enum
	{
		array_exprs,
		array_str
	} type;
};

struct expr
{
	where where;

	enum expr_type
	{
		expr_op,
		expr_val,
		expr_addr, /* &x, or string/array pointer */
		expr_sizeof,
		expr_identifier,
		expr_assign,
		expr_funcall,
		expr_struct,
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

		op_shiftl, op_shiftr,

		op_unknown
	} op;

	int assign_is_post; /* do we return the altered value or the old one? */
#define expr_is_default assign_is_post
#define expr_is_sizeof  assign_is_post

	expr *lhs, *rhs;

	union
	{
		int i;
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

		stat_label,
		stat_switch,
		stat_case,
		stat_case_range,
		stat_default,

		stat_noop
	} type;

	tree *lhs, *rhs;
	expr *expr; /* test expr for if and do, etc */

	tree_flow *flow; /* for, switch (do and while are simple enough for ->[lr]hs) */

	/* specific data */
	int val;
	char *lblfin;
	function *func;
	decl **decls; /* block definitions, e.g. { int i... } */
	tree **codes; /* for a code block */

	symtable *symtab; /* pointer to the containing function's symtab */
};

struct tree_flow
{
	expr *for_init, *for_while, *for_inc;
};

struct function
{
	where where;

	int args_void; /* true if "spel(void);" otherwise if !args, then we have "spel();" */
	decl **args;
	tree *code;
	int variadic;
};

tree        *tree_new(void);
expr        *expr_new(void);
type        *type_new(void);
decl        *decl_new(void);
decl        *decl_new_where(where *);
array_decl  *array_decl_new(void);
function    *function_new(void);

type      *type_copy(type *);
decl      *decl_copy(decl *);
/*expr      *expr_copy(expr *);*/
tree      *tree_new_code(void);
expr      *expr_new_val(int);

tree_flow *tree_flow_new(void);

const char *op_to_str(  const enum op_type   o);
const char *expr_to_str(const enum expr_type t);
const char *stat_to_str(const enum stat_type t);
const char *decl_to_str(const decl          *d);
const char *type_to_str(const type          *t);
const char *spec_to_str(const enum type_spec s);

int   type_equal(const type *a, const type *b, int strict);
int   type_size(const type *);
int   decl_size(const decl *);
int   decl_equal(const decl *, const decl *, int strict);
expr *expr_ptr_multiply(expr *, decl *);
expr *expr_assignment(expr *to, expr *from);
void function_empty_args(function *);

#define TYPE_STATIC_BUFSIZ 64
#define DECL_STATIC_BUFSIZ (32 + TYPE_STATIC_BUFSIZ)

#define type_free(x) free(x)
#define decl_free(x) do{type_free((x)->type); free(x);}while(0)
#define expr_free(x) do{decl_free((x)->tree_type); free(x);}while(0)

#endif
