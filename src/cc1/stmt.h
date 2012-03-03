#ifndef STAT_H
#define STAT_H

typedef void        func_fold_stmt(stmt *);
typedef void        func_gen_stmt( stmt *);
typedef const char *func_str_stmt(void);

struct stmt
{
	where where;

	func_fold_stmt *f_fold;
	func_gen_stmt  *f_gen;
	func_str_stmt  *f_str;

	stmt *lhs, *rhs;
	expr *expr; /* test expr for if and do, etc */
	expr *expr2;

	stmt_flow *flow; /* for, switch (do and while are simple enough for ->[lr]hs) */

	/* specific data */
	int val;
	char *lblfin;

	decl **decls; /* block definitions, e.g. { int i... } */
	stmt **codes; /* for a code block */

	symtable *symtab; /* pointer to the containing funcargs's symtab */
};

struct stmt_flow
{
	expr *for_init, *for_while, *for_inc;
};

#include "ops/stmt_break.h"
#include "ops/stmt_case.h"
#include "ops/stmt_case_range.h"
#include "ops/stmt_code.h"
#include "ops/stmt_default.h"
#include "ops/stmt_do.h"
#include "ops/stmt_expr.h"
#include "ops/stmt_for.h"
#include "ops/stmt_goto.h"
#include "ops/stmt_if.h"
#include "ops/stmt_label.h"
#include "ops/stmt_noop.h"
#include "ops/stmt_return.h"
#include "ops/stmt_switch.h"
#include "ops/stmt_while.h"

#define stmt_new_wrapper(type, stab) stmt_new(fold_stmt_ ## type, gen_stmt_ ## type, str_stmt_ ## type, stab)
#define stmt_mutate_wrapper(s, type)    stmt_mutate(s, fold_stmt_ ## type, gen_stmt_ ## type, str_stmt_ ## type)

#define stmt_kind(st, kind) ((st)->f_fold == fold_stmt_ ## kind)

stmt *stmt_new(func_fold_stmt *, func_gen_stmt *, func_str_stmt *, symtable *stab);
stmt_flow *stmt_flow_new(void);

#define stmt_new_code(stab)         stmt_new_wrapper(code,    stab)
#define stmt_new_do(stab)           stmt_new_wrapper(do,      stab)
#define stmt_new_expr(stab)         stmt_new_wrapper(expr,    stab)
#define stmt_new_for(stab)          stmt_new_wrapper(for,     stab)
#define stmt_new_goto(stab)         stmt_new_wrapper(goto,    stab)
#define stmt_new_if(stab)           stmt_new_wrapper(if,      stab)
#define stmt_new_label(stab)        stmt_new_wrapper(label,   stab)
#define stmt_new_noop(stab)         stmt_new_wrapper(noop,    stab)
#define stmt_new_return(stab)       stmt_new_wrapper(return,  stab)
#define stmt_new_switch(stab)       stmt_new_wrapper(switch,  stab)
#define stmt_new_while(stab)        stmt_new_wrapper(while,   stab)

stmt *stmt_new_case(symtable *);
stmt *stmt_new_case_range(symtable *);
stmt *stmt_new_default(symtable *);
stmt *stmt_new_break(symtable *);

void stmt_mutate(stmt *, func_fold_stmt *, func_gen_stmt *, func_str_stmt *);

#endif
