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

void fold_stat_and_add_to_curswitch(stat *);


void fold_test_expr(expr *e, const char *stat_desc);

void fold_expr(expr *e, symtable *stab);
void fold_stat(stat *t);

void fold(symtable *);

extern char *curdecl_func_sp;
extern stat *curstat_flow;
extern stat *curstat_switch;

#endif
