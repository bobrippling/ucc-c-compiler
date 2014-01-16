#ifndef PARSE_H
#define PARSE_H

#include "expr.h"
#include "stmt.h"

enum decl_mode
{
	DECL_SPEL_NEED    = 1 << 0,
	DECL_CAN_DEFAULT  = 1 << 1,
	DECL_ALLOW_STORE  = 1 << 2
};

enum decl_multi_mode
{
	DECL_MULTI_CAN_DEFAULT        = 1 << 0,
	DECL_MULTI_ACCEPT_FIELD_WIDTH = 1 << 1,
	DECL_MULTI_ACCEPT_FUNC_DECL   = 1 << 2,
	DECL_MULTI_ACCEPT_FUNC_CODE   = 1 << 3 | DECL_MULTI_ACCEPT_FUNC_DECL,
	DECL_MULTI_ALLOW_STORE        = 1 << 4,
	DECL_MULTI_NAMELESS           = 1 << 5,
	DECL_MULTI_ALLOW_ALIGNAS      = 1 << 6,
};

extern enum token curtok;
extern symtable *current_scope;

#define parse_expr_no_comma() parse_expr_assignment()

/* these need to be visible to parse_type.c */
expr *parse_expr_assignment(void);
stmt *parse_stmt_block(void);
expr *parse_expr_sizeof_typeof_alignof(enum what_of what_of);
expr *parse_expr_exp(void);

void parse_static_assert(void);
type **parse_type_list(void);
expr **parse_funcargs(void);
symtable_gasm *parse_gasm(void);

#endif
