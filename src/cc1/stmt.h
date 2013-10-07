#ifndef STAT_H
#define STAT_H

typedef void        func_fold_stmt(stmt *);
typedef void        func_gen_stmt( stmt *);
typedef const char *func_str_stmt(void);

/* non-critical */
typedef int         func_passable_stmt(stmt *);

struct stmt
{
	where where;

	func_fold_stmt     *f_fold;
	func_gen_stmt      *f_gen;
	func_str_stmt      *f_str;
	func_passable_stmt *f_passable; /* can code get past this statement:
	                                   no for return + things containing return, etc */

	stmt *lhs, *rhs;
	expr *expr; /* test expr for if and do, etc */
	expr *expr2;

	stmt_flow *flow; /* for, switch (do and while are simple enough for ->[lr]hs) */

	/* specific data */
	int val;
	char *lbl_break, *lbl_continue;

	int freestanding;     /* if this is freestanding, non-freestanding expressions inside are allowed */
	int kills_below_code; /* break, return, etc - for checking dead code */
	int expr_no_pop;

	struct
	{
		struct
		{
			struct label *label;
			char *spel;
			int unused;
		} lbl;
	} bits;

	stmt **codes; /* for a code block */

	symtable *symtab; /* block definitions, e.g. { int i... } */

	/* parents - applicable for break and continue */
	stmt *parent;
};

struct stmt_flow
{
	symtable *for_init_symtab; /* for(int b;;){} - symtab for b */
	stmt *init_blk;

	/* for specific */
	expr *for_init, *for_while, *for_inc;
};

#define STMT_DEFS(ty)                  \
	func_fold_stmt   fold_stmt_ ## ty;   \
	func_str_stmt    str_stmt_ ## ty;    \
	func_gen_stmt    style_stmt_ ## ty;  \
	func_gen_stmt    gen_stmt_ ## ty;    \
	void   init_stmt_ ## ty(stmt *)

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

#define stmt_new_wrapper(type, stab) stmt_new(                \
                                        fold_stmt_ ## type,   \
                                        gen_stmt_ ## type,    \
                                        style_stmt_ ## type,  \
                                        str_stmt_ ## type,    \
                                        init_stmt_ ## type,   \
                                        stab)

#define stmt_kind(st, kind) ((st)->f_fold == fold_stmt_ ## kind)

stmt *stmt_new(func_fold_stmt *,
		func_gen_stmt *g_asm,
		func_gen_stmt *g_style,
		func_str_stmt *,
		void (*init)(stmt *),
		symtable *stab);

stmt_flow *stmt_flow_new(symtable *parent);

stmt *expr_to_stmt(expr *e, symtable *scope);

typedef void stmt_walk_enter(stmt *current, int *stop, int *descend, void *);
typedef void stmt_walk_leave(stmt *current, void *);

stmt_walk_enter stmt_walk_first_return; /* completes after the first return statement is found */

void stmt_walk(stmt *base, stmt_walk_enter, stmt_walk_leave, void *data);

#endif
