#ifndef EXPR_ASSIGN_H
#define EXPR_ASSIGN_H

EXPR_DEFS(assign);

int expr_must_lvalue(expr *e, const char *desc);
void bitfield_trunc_check(decl *mem, expr *from);
void expr_assign_const_check(expr *e, where *w);

expr *expr_new_assign(         expr *to, expr *from);
expr *expr_new_assign_init(    expr *to, expr *from);
expr *expr_new_assign_compound(expr *to, expr *from, enum op_type);

#endif
