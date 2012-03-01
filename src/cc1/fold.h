#ifndef FOLD_H
#define FOLD_H

void fold_decl_equal(
		decl *a, decl *b,
		where *w,
		enum warning warn, const char *errfmt, ...);

void fold_expr(expr *e, symtable *stab);
void fold_tree(tree *t);

void fold_funcargs(funcargs *fargs, symtable *stab, char *context);
void fold_funcargs_equal(funcargs *args_a, funcargs *args_b,
		int check_vari,
		where *w, const char *warn_pre,
		const char *func_spel);

void fold(symtable *);


extern char *curdecl_func_sp;

#endif
