typedef struct sym         sym;
typedef struct symtable    symtable;

typedef struct tdef        tdef;
typedef struct tdeftable   tdeftable;
typedef struct struct_st   struct_st;
typedef struct enum_st     enum_st;

typedef struct tree        tree;
typedef struct decl        decl;
typedef struct decl_ptr    decl_ptr;
typedef struct array_decl  array_decl;
typedef struct funcargs    funcargs;
typedef struct tree_flow   tree_flow;
typedef struct type        type;
typedef struct assignment  assignment;
typedef struct label       label;

typedef struct intval intval;

struct intval
{
	long val;
	enum
	{
		VAL_UNSIGNED = 1 << 0,
		VAL_LONG     = 1 << 1
	} suffix;
};


#include "expr.h"
#include "sym.h"
#include "tree.h"
