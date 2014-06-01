#ifndef PARSE_EXPR_H
#define PARSE_EXPR_H

#include "expr.h"

/* these need to be visible to parse_type.c */
#define PARSE_EXPR_NO_COMMA(s, static_ctx) \
	parse_expr_assignment(s, static_ctx)

expr *parse_expr_assignment(symtable *, int static_ctx);

expr *parse_expr_sizeof_typeof_alignof(
		enum what_of what_of, symtable *scope);

expr *parse_expr_exp(symtable *, int static_ctx);

expr **parse_funcargs(symtable *, int static_ctx);
symtable_gasm *parse_gasm(void);

#endif
