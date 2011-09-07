#ifndef token_ise_h
#define token_ise_h

enum token
{
	token_do,
	token_if,
	token_else,
	token_while,
	token_for,
	token_break,
	token_return,

	token_switch,
	token_case,
	token_default,

	token_sizeof,
	token_extern,

	token_identifier,
	token_integer,     /* aka [1-9] */
	token_character,   /* aka 'f' */
	token_void,        /* aka subroutine */
	token_char,        /* aka "char" */
	token_int,         /* aka "int" */
	token_elipsis,     /* aka ... */
	token_string,      /* aka \"...\" */
	token_const,

	token_open_paren,
	token_open_block,
	token_open_square,

	token_close_paren,
	token_close_block,
	token_close_square,

	token_comma,
	token_semicolon,
	token_colon,

	token_plus,
	token_minus,
	token_multiply,
	token_divide,
	token_modulus,
	token_increment,
	token_decrement,

	token_assign,
	token_dot,

	token_eq,
	token_le,
	token_lt,
	token_ge,
	token_gt,
	token_ne,
	token_not,
	token_bnot,
	token_andsc,
	token_and,
	token_orsc,
	token_or,

	token_eof,
	token_unknown
};

void tokenise_set_file(FILE *f, const char *nam);
void nexttoken();

#endif
