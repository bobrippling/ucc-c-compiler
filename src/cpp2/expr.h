#ifndef EXPR_H
#define EXPR_H

typedef struct expr expr;
typedef long expr_n;

expr *expr_parse(char *);
expr_n expr_eval(expr *, int *had_ident, int noop);

#endif
