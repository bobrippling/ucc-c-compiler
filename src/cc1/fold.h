#ifndef FOLD_H
#define FOLD_H

void fold_decl(decl *d, symtable *stab);

void fold_decl_equal(
		decl *a, decl *b,
		where *w,
		enum warning warn, const char *errfmt, ...);

void fold_funcargs(funcargs *fargs, symtable *stab, char *context);

void fold_funcargs_equal(
		funcargs *args_a, funcargs *args_b,
		int check_vari,
		where *w, const char *warn_pre,
		const char *func_spel);

void fold_symtab_scope(symtable *stab);

void fold_stmt_and_add_to_curswitch(stmt *);

void fold_insert_casts(decl *dlhs, expr **prhs, symtable *stab, where *where);

void fold_test_expr(expr *e, const char *stmt_desc);
void fold_disallow_st_un(expr *e, const char *desc);

int  fold_get_sym(          expr *e, symtable *stab);
void fold_inc_writes_if_sym(expr *e, symtable *stab);

void fold_coerce_assign(decl *d, expr *assign, int *ok);

void fold_expr(expr *e, symtable *stab);
void fold_stmt(stmt *t);

void fold(symtable *);

extern decl *curdecl_func;
extern stmt *curstmt_flow;
extern stmt *curstmt_switch;

#endif
