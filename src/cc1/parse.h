#ifndef PARSE_H
#define PARSE_H

enum decl_mode
{
	DECL_SPEL_NEED    = 1 << 0,
	DECL_SPEL_NO      = 1 << 1,
	DECL_CAN_DEFAULT  = 1 << 2,
	DECL_NO_BARE_FUNC = 1 << 3, /* accept "int (char)" as func? (for block ret-type parsing) */
};

enum decl_multi_mode
{
	DECL_MULTI_CAN_DEFAULT        = 1 << 0,
	DECL_MULTI_ACCEPT_FIELD_WIDTH = 1 << 1,
	DECL_MULTI_ACCEPT_FUNC_DECL   = 1 << 2,
	DECL_MULTI_ACCEPT_FUNC_CODE   = 1 << 3 | DECL_MULTI_ACCEPT_FUNC_DECL,
};

extern enum token curtok;

#define parse_possible_decl()      \
		(  curtok == token_identifier  \
		|| curtok == token_multiply    \
		|| curtok == token_xor         \
		|| curtok == token_open_paren)

#define parse_expr_no_comma() parse_expr_assignment()

/* these need to be visible to parse_type.c */
expr *parse_expr_assignment(void);
stmt *parse_code_block(void);
expr *parse_expr_sizeof_typeof(void);
expr *parse_expr_exp(void);

void parse_static_assert(void);

symtable *parse(void);

/* tables local to the current scope */
extern symtable *current_scope;

#endif
