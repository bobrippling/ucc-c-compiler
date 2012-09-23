#ifndef FOLD_H
#define FOLD_H

void fold_decl(decl *d, symtable *stab);
void fold_decl_global_init(decl_init *dinit, symtable *stab);

void fold_decl_equal(
		decl *a, decl *b,
		where *w,
		enum warning warn, const char *errfmt, ...);

void fold_funcargs(funcargs *fargs, symtable *stab, char *context);

void fold_symtab_scope(symtable *stab);

void fold_insert_casts(decl *dlhs, expr **prhs, symtable *stab, where *w, const char *desc);
void fold_typecheck(expr *lhs, expr *rhs, symtable *stab, where *where);

void fold_need_expr(expr *e, const char *stmt_desc, int is_test);
void fold_disallow_st_un(expr *e, const char *desc);

void fold_gen_init_assignment(decl *dfor, stmt *code);

int  fold_get_sym(          expr *e, symtable *stab);
void fold_inc_writes_if_sym(expr *e, symtable *stab);

void fold_expr(expr *e, symtable *stab);
void fold_stmt(stmt *t);

int fold_passable(stmt *s);
int fold_passable_yes(stmt *s);
int fold_passable_no( stmt *s);

void fold(symtable *);

extern decl *curdecl_func, *curdecl_func_called;

void fold_stmt_and_add_to_curswitch(stmt *);

#ifdef SYMTAB_DEBUG
#  define PRINT_STAB(st, cur) print_stab(st->symtab, cur, &st->where)
void print_stab(symtable *st, int current, where *w);
#endif

#endif
