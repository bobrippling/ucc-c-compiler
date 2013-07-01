#ifndef FOLD_H
#define FOLD_H

void fold_decl(decl *d, symtable *stab);
void fold_decl(decl *d, symtable *stab);
void fold_decl_global_init(decl *d, symtable *stab);
void fold_type_ref(type_ref *r, type_ref *parent, symtable *stab);

int fold_type_ref_equal(
		type_ref *a, type_ref *b, where *w,
		enum warning warn, enum decl_cmp extra_flags,
		const char *errfmt, ...);

void fold_check_restrict(expr *lhs, expr *rhs, const char *desc, where *w);

void fold_symtab_scope(symtable *stab, stmt **pinit_code);

void fold_funcargs(funcargs *fargs, symtable *stab, type_ref *from);

void fold_insert_casts(type_ref *dlhs, expr **prhs, symtable *stab, where *w, const char *desc);

enum fold_chk
{
	/* implicitly must be non-void */
	FOLD_CHK_EXP         = 0,      /* done for all */
	FOLD_CHK_NO_ST_UN    = 1 << 0, /* e.g. struct A + ... */
	FOLD_CHK_NO_BITFIELD = 1 << 1, /* e.g. &, sizeof */
	FOLD_CHK_BOOL        = 1 << 2, /* e.g. if(...) */
	FOLD_CHK_INTEGRAL    = 1 << 3, /* e.g. switch(...) */
};
void fold_check_expr(expr *e, enum fold_chk, const char *desc);

/*
void fold_need_expr(expr *e, const char *stmt_desc, int is_test);
void fold_disallow_st_un(expr *e, const char *desc);
void fold_disallow_bitfield(expr *e, const char *desc, ...);
*/

sym *fold_inc_writes_if_sym(expr *e, symtable *stab);

#define FOLD_EXPR(e, stab) ((e) = fold_expr((e), (stab)))
void FOLD_EXPR_NO_DECAY(expr *e, symtable *stab); /* for unary-& and sizeof */
expr *fold_expr(expr *e, symtable *stab) ucc_wur;
void fold_stmt(stmt *t);

int fold_passable(stmt *s);
int fold_passable_yes(stmt *s);
int fold_passable_no( stmt *s);

void fold(symtable *);

extern decl *curdecl_func;
extern type_ref *curdecl_ref_func_called;
extern int fold_had_error;

void fold_stmt_and_add_to_curswitch(stmt *);

#ifdef SYMTAB_DEBUG
#  define PRINT_STAB(st, cur) print_stab(st->symtab, cur, &st->where)
void print_stab(symtable *st, int current, where *w);
#endif

#endif
