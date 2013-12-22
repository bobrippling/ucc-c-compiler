#ifndef token_ise_h
#define token_ise_h

extern numeric currentval;

enum token
{
	token_do,
	token_if,
	token_else,
	token_while,
	token_for,
	token_break,
	token_continue,
	token_return,

	token_switch,
	token_case,
	token_default,
	token_goto,

	token__Generic,
	token_sizeof,
	token_typeof,
	token__Static_assert,

	token_asm,
	token_attribute,
	token_extension,

	token_identifier,
	token_integer,     /* aka [1-9] */
	token_floater,     /* aka [1-9].[0-9]... */
	token_character,   /* aka 'f' */
	token_elipsis,     /* aka ... */
	token_string,      /* aka \"...\" */

	token_void,        /* aka subroutine */
	token_char,        /* aka "char" */
	token_short,
	token_int,         /* aka "int" */
	token_long,
	token_float,
	token_double,
	token__Bool,
	token___builtin_va_list,

	token_inline,
	token__Noreturn,

	token_const,
	token_volatile,
	/**/
	token_restrict, /* sort of a type-qual */
	/**/
	token_signed,
	token_unsigned,
	/**/
	token_auto,
	token_static,
	token_extern,
	token_register,
	token__Alignof,
	token__Alignas,
	/**/
	token_typedef,
	token_struct,
	token_union,
	token_enum,

	token_open_paren,
	token_open_block,
	token_open_square,

	token_close_paren,
	token_close_block,
	token_close_square,

	token_comma,
	token_semicolon,
	token_colon,
	token_question,

#define TOK_AND_EQ(x) x, x##_assign
	TOK_AND_EQ(token_plus),
	TOK_AND_EQ(token_minus),
	TOK_AND_EQ(token_multiply),
	TOK_AND_EQ(token_divide),
	TOK_AND_EQ(token_modulus),
	TOK_AND_EQ(token_not),
	TOK_AND_EQ(token_bnot),
	TOK_AND_EQ(token_and),
	TOK_AND_EQ(token_or),
	TOK_AND_EQ(token_xor),
	TOK_AND_EQ(token_shiftl),
	TOK_AND_EQ(token_shiftr),
#undef TOK_AND_EQ

	token_increment,
	token_decrement,

	token_assign,
	token_dot,
	token_ptr, /* -> */

	token_eq,
	token_le,
	token_lt,
	token_ge,
	token_gt,
	token_ne,
	token_andsc,
	token_orsc,

	token_eof,
	token_unknown
};

typedef char *tokenise_line_f(void);

void tokenise_set_input(
		tokenise_line_f *,
		const char *nam);

void nexttoken(void);

char *token_current_spel(void);
char *token_current_spel_peek(void);

int tok_at_label(void);

#endif
