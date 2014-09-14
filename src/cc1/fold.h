#ifndef FOLD_H
#define FOLD_H

#include "decl.h"
#include "sym.h"
#include "stmt.h"
#include "expr.h"
#include "funcargs.h"

/* basic folding */
void fold_decl_global(decl *d, symtable *stab);
void fold_decl_global_init(decl *d, symtable *stab);

void fold_merge_tenatives(symtable *stab);

void fold_decl_add_sym(decl *d, symtable *stab);

void fold_decl(decl *d, symtable *stab);
void fold_global_func(decl *);
void fold_func_code(stmt *code, where *w, char *sp, symtable *arg_symtab);
int fold_func_is_passable(decl *, type *, int warn);

void fold_type(type *t, symtable *stab);
void fold_type_w_attr(
		type *, type *parent, where *,
		symtable *stab, attribute *attr);

void fold_check_restrict(expr *lhs, expr *rhs, const char *desc, where *w);

void fold_funcargs(funcargs *fargs, symtable *stab, attribute *);

/* cast insertion */
void fold_insert_casts(type *tlhs, expr **prhs, symtable *stab);

int fold_type_chk_warn(
		type *lhs, type *rhs,
		where *w, const char *desc);

void fold_type_chk_and_cast(
		type *lhs, expr **prhs,
		symtable *stab, where *w,
		const char *desc);

/* struct / enum / integral checking */
enum fold_chk
{
	FOLD_CHK_NO_ST_UN    = 1 << 0, /* e.g. struct A + ... */
	FOLD_CHK_NO_BITFIELD = 1 << 1, /* e.g. &, sizeof */
	FOLD_CHK_BOOL        = 1 << 2, /* e.g. if(...) */
	FOLD_CHK_INTEGRAL    = 1 << 3, /* e.g. switch(...) */
	FOLD_CHK_ALLOW_VOID  = 1 << 4,
	FOLD_CHK_CONST_I     = 1 << 5, /* e.g. case (...): */
	FOLD_CHK_NOWARN_ASSIGN = 1 << 6, /* if(a = b){ ... } */
	FOLD_CHK_ARITHMETIC = 1 << 7,
};
void fold_check_expr(expr *e, enum fold_chk, const char *desc);

/* expression + statement folding */
expr *fold_expr_decay(expr *e, symtable *stab) ucc_wur;
void fold_expr(expr *e, symtable *stab);
#define FOLD_EXPR(e, stab) ((e) = fold_expr_decay((e), (stab)))
#define fold_expr_no_decay fold_expr

void fold_stmt(stmt *t);

sym *fold_inc_writes_if_sym(expr *e, symtable *stab);

/* flow */
int fold_passable(stmt *s);
int fold_passable_yes(stmt *s);
int fold_passable_no( stmt *s);

extern int fold_had_error;

#endif
