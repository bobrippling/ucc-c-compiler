typedef struct funcargs    funcargs;
typedef struct tree_flow   tree_flow;

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

typedef const char *func_str(void);

#include "tree.h"
#include "expr.h"
#include "stmt.h"
#include "op.h"
#include "sym.h"
