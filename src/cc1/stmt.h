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
	char *lbl_break, *lbl_continue;

	int freestanding;     /* if this is freestanding, non-freestanding expressions inside are allowed */
	int kills_below_code; /* break, return, etc - for checking dead code */

	decl **decls; /* block definitions, e.g. { int i... } */
	stmt **codes; /* for a code block */

	symtable *symtab;

	/* parents - applicable for break and continue */
	stmt *parent;
};

struct stmt_flow
{
	expr *for_init, *for_while, *for_inc;

	decl    **for_init_decls;  /* c99 for initialisation (and ucc if-init) */
	symtable *for_init_symtab; /* for(int b;;){} - symtab for b */
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
#include "ops/stmt_continue.h"

#define stmt_new_wrapper(type, stab) stmt_new(fold_stmt_ ## type, gen_stmt_ ## type, str_stmt_ ## type, stab)
#define stmt_mutate_wrapper(s, type)    stmt_mutate(s, fold_stmt_ ## type, gen_stmt_ ## type, str_stmt_ ## type)

#define stmt_kind(st, kind) ((st)->f_fold == fold_stmt_ ## kind)

stmt *stmt_new(func_fold_stmt *, func_gen_stmt *, func_str_stmt *, symtable *stab);
stmt_flow *stmt_flow_new(symtable *parent);
void stmt_mutate(stmt *, func_fold_stmt *, func_gen_stmt *, func_str_stmt *);

void stmt_walk(stmt *base, void (*f)(stmt *current, int *stop, void *), void *data);

#endif
