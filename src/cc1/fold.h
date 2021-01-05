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
void fold_decl_maybe_member(decl *d, symtable *stab, int su_member);
void fold_decl_attrs_requiring_fnbody(decl *d, int su_member);

void fold_check_decl_complete(decl *d);
void fold_global_func(decl *);
void fold_func_code(stmt *code, where *w, char *sp, symtable *arg_symtab);
int fold_func_is_passable(decl *, type *, int warn);

/* unadorned types, e.g. (int)..., _Generic(..., int: ...) */
void fold_type(type *t, symtable *stab);

/* type as part of something else, e.g. int x; */
void fold_type_ondecl_w(decl *, symtable *, const where *, int is_arg);

void fold_check_restrict(expr *lhs, expr *rhs, const char *desc, where *w);

void fold_check_embedded_flexar(
		struct struct_union_enum_st *, const where *, const char *desc);

void fold_funcargs(funcargs *fargs, symtable *stab, attribute **);

int fold_get_max_align_attribute(attribute **attribs, symtable *stab, const int min);

/* cast insertion */
void fold_insert_casts(type *tlhs, expr **prhs, symtable *stab);

int fold_type_chk_warn(
		/* take exprs to check null-ptr-constants */
		expr *maybe_lhs,
		type *tlhs,
		expr *rhs,
		int is_comparison,
		where *w, const char *desc);

void fold_type_chk_and_cast_ty(
		type *tlhs, expr **prhs,
		symtable *stab, where *w,
		const char *desc);

void fold_type_chk_and_cast(
		expr *lhs, expr **prhs,
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

ucc_wur
int fold_check_expr(const expr *e, enum fold_chk, const char *desc);

/* expression + statement folding */
/*   decay */
expr *fold_expr_lval2rval(expr *e, symtable *stab) ucc_wur;
#define FOLD_EXPR(e, stab) ((e) = fold_expr_lval2rval((e), (stab)))
expr *fold_expr_nonstructdecay(expr *e, symtable *stab) ucc_wur;
/*   normal fold */
void fold_expr_nodecay(expr *e, symtable *stab);

void fold_stmt(stmt *t);

sym *fold_inc_writes_if_sym(expr *e, symtable *stab);

/* flow */
int fold_passable(stmt *s, int break_means_passable);
int fold_passable_yes(stmt *s, int break_means_passable);
int fold_passable_no( stmt *s, int break_means_passable);

#include "parse_fold_error.h"

#endif
