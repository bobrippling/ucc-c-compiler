#ifndef EXPR_TOK_H
#define EXPR_TOK_H

extern expr_n tok_cur_num;

extern enum tok
{
	tok_ident = -1,
	tok_num   = -2,
	tok_eof   =  0,

	tok_lparen = '(',
	tok_rparen = ')',

	/* operators returned as char-value,
	 * except for double-char ops
	 */

	/* binary */
	tok_multiply = '*',
	tok_divide   = '/',
	tok_modulus  = '%',
	tok_plus     = '+',
	tok_minus    = '-',
	tok_xor      = '^',
	tok_or       = '|',
	tok_and      = '&',
	tok_orsc     = -3,
	tok_andsc    = -4,
	tok_shiftl   = -5,
	tok_shiftr   = -6,

	/* unary - TODO */
	tok_not      = '!',
	tok_bnot     = '~',

	/* comparison */
	tok_eq       = -7,
	tok_ne       = -8,
	tok_le       = -9,
	tok_lt       = '<',
	tok_ge       = -10,
	tok_gt       = '>',

	/* ternary */
	tok_question = '?',
	tok_colon    = ':',

#define MIN_OP -10
} tok_cur;

void tok_next(void);
void tok_begin(char *);
const char *tok_last(void);

#endif
