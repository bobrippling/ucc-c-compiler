#ifndef STAT_H
#define STAT_H

typedef void        func_fold_stat(stat *);
typedef void        func_gen_stat( stat *);
typedef const char *func_str_stat(void);

struct stat
{
	where where;

	func_fold_stat *f_fold;
	func_gen_stat  *f_gen;
	func_str_stat  *f_str;

	stat *lhs, *rhs;
	expr *expr; /* test expr for if and do, etc */
	expr *expr2;

	stat_flow *flow; /* for, switch (do and while are simple enough for ->[lr]hs) */

	/* specific data */
	int val;
	char *lblfin;

	decl **decls; /* block definitions, e.g. { int i... } */
	stat **codes; /* for a code block */

	symtable *symtab; /* pointer to the containing funcargs's symtab */
};

struct stat_flow
{
	expr *for_init, *for_while, *for_inc;
};

#include "ops/stat_break.h"
#include "ops/stat_case.h"
#include "ops/stat_case_range.h"
#include "ops/stat_code.h"
#include "ops/stat_default.h"
#include "ops/stat_do.h"
#include "ops/stat_expr.h"
#include "ops/stat_for.h"
#include "ops/stat_goto.h"
#include "ops/stat_if.h"
#include "ops/stat_label.h"
#include "ops/stat_noop.h"
#include "ops/stat_return.h"
#include "ops/stat_switch.h"
#include "ops/stat_while.h"

#define stat_new_wrapper(type, stab) stat_new(fold_stat_ ## type, gen_stat_ ## type, str_stat_ ## type, stab)
#define stat_mutate_wrapper(s, type)    stat_mutate(s, fold_stat_ ## type, gen_stat_ ## type, str_stat_ ## type)

#define stat_kind(st, kind) ((st)->f_fold == fold_stat_ ## kind)

stat *stat_new(func_fold_stat *, func_gen_stat *, func_str_stat *, symtable *stab);
stat_flow *stat_flow_new(void);

#define stat_new_code(stab)         stat_new_wrapper(code,    stab)
#define stat_new_do(stab)           stat_new_wrapper(do,      stab)
#define stat_new_expr(stab)         stat_new_wrapper(expr,    stab)
#define stat_new_for(stab)          stat_new_wrapper(for,     stab)
#define stat_new_goto(stab)         stat_new_wrapper(goto,    stab)
#define stat_new_if(stab)           stat_new_wrapper(if,      stab)
#define stat_new_label(stab)        stat_new_wrapper(label,   stab)
#define stat_new_noop(stab)         stat_new_wrapper(noop,    stab)
#define stat_new_return(stab)       stat_new_wrapper(return,  stab)
#define stat_new_switch(stab)       stat_new_wrapper(switch,  stab)
#define stat_new_while(stab)        stat_new_wrapper(while,   stab)

stat *stat_new_case(symtable *);
stat *stat_new_case_range(symtable *);
stat *stat_new_default(symtable *);
stat *stat_new_break(symtable *);

void stat_mutate(stat *, func_fold_stat *, func_gen_stat *, func_str_stat *);

#endif
