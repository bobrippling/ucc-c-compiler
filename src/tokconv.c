#include <stdio.h>
#include <stdarg.h>

#include "tokenise.h"
#include "tree.h"
#include "tokconv.h"
#include "util.h"
#include "macros.h"

extern enum token curtok;

enum type curtok_to_type()
{
	switch(curtok){
		case token_int:  return type_int;
		case token_byte: return type_byte;
		case token_void: return type_void;
		default: break;
	}
	return type_unknown;
}

enum op_type curtok_to_op()
{
	switch(curtok){
		/* multiply - op_deref is handled by the parser */
		case token_multiply: return op_multiply;

		case token_divide: return op_divide;
		case token_plus: return op_plus;
		case token_minus: return op_minus;
		case token_modulus: return op_modulus;

		case token_eq: return op_eq;
		case token_ne: return op_ne;
		case token_le: return op_le;
		case token_lt: return op_lt;
		case token_ge: return op_ge;
		case token_gt: return op_gt;

		case token_or: return op_or;
		case token_and: return op_and;
		case token_orsc: return op_orsc;
		case token_andsc: return op_andsc;
		case token_not: return op_not;
		case token_bnot: return op_bnot;

		default: break;
	}

	return op_unknown;
}

int curtok_is_type()
{
	return curtok_to_type() != type_unknown;
}

const char *token_to_str(enum token t)
{
	switch(t){
		CASE_STR(token_do);           CASE_STR(token_if);            CASE_STR(token_else);         CASE_STR(token_while);
		CASE_STR(token_for);          CASE_STR(token_break);         CASE_STR(token_return);       CASE_STR(token_switch);
		CASE_STR(token_case);         CASE_STR(token_default);       CASE_STR(token_sizeof);       CASE_STR(token_extern);
		CASE_STR(token_identifier);   CASE_STR(token_integer);       CASE_STR(token_character);    CASE_STR(token_void);
		CASE_STR(token_byte);         CASE_STR(token_int);           CASE_STR(token_elipsis);      CASE_STR(token_string);
		CASE_STR(token_open_paren);   CASE_STR(token_open_block);    CASE_STR(token_open_square);  CASE_STR(token_close_paren);
		CASE_STR(token_close_block);  CASE_STR(token_close_square);  CASE_STR(token_comma);        CASE_STR(token_semicolon);
		CASE_STR(token_colon);        CASE_STR(token_plus);          CASE_STR(token_minus);        CASE_STR(token_multiply);
		CASE_STR(token_divide);       CASE_STR(token_modulus);       CASE_STR(token_increment);    CASE_STR(token_decrement);
		CASE_STR(token_assign);       CASE_STR(token_dot);           CASE_STR(token_eq);           CASE_STR(token_le);
		CASE_STR(token_lt);           CASE_STR(token_ge);            CASE_STR(token_gt);           CASE_STR(token_ne);
		CASE_STR(token_not);          CASE_STR(token_bnot);          CASE_STR(token_andsc);        CASE_STR(token_and);
		CASE_STR(token_orsc);         CASE_STR(token_or);            CASE_STR(token_eof);          CASE_STR(token_unknown);
	}
	return NULL;
}

void eat(enum token t)
{
	if(t != curtok)
		die_at("expecting token %s, got %s",
				token_to_str(t), token_to_str(curtok));
	nexttoken();
}

int curtok_in_list(va_list l)
{
	enum token t;
	while((t = va_arg(l, enum token)) != token_unknown)
		if(curtok == t)
			return 1;
	return 0;
}

#define NULL_AND_RET(fnam, cnam)  \
char *fnam()                      \
{                                 \
	extern char *cnam;              \
	char *ret = cnam;               \
	cnam = NULL;                    \
	return ret;                     \
}

NULL_AND_RET(token_current_spel, currentspelling)
NULL_AND_RET(token_current_str,  currentstring)
