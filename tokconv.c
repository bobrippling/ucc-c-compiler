#include <stdio.h>

#include "tokenise.h"
#include "tree.h"
#include "tokconv.h"
#include "util.h"

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

enum expr_op curtok_to_op()
{
	switch(curtok){
		/*
		 * case token_multiply: return deref ? op_deref : op_multiply;
		 *
		 * doesn't matter
		 */

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
#define STR(s) case s: return #s
	switch(t){
		STR(token_do);           STR(token_if);            STR(token_else);         STR(token_while);
		STR(token_for);          STR(token_break);         STR(token_return);       STR(token_switch);
		STR(token_case);         STR(token_default);       STR(token_sizeof);       STR(token_extern);
		STR(token_identifier);   STR(token_integer);       STR(token_character);    STR(token_void);
		STR(token_byte);         STR(token_int);           STR(token_elipsis);      STR(token_string);
		STR(token_open_paren);   STR(token_open_block);    STR(token_open_square);  STR(token_close_paren);
		STR(token_close_block);  STR(token_close_square);  STR(token_comma);        STR(token_semicolon);
		STR(token_colon);        STR(token_plus);          STR(token_minus);        STR(token_multiply);
		STR(token_divide);       STR(token_modulus);       STR(token_increment);    STR(token_decrement);
		STR(token_assign);       STR(token_dot);           STR(token_eq);           STR(token_le);
		STR(token_lt);           STR(token_ge);            STR(token_gt);           STR(token_ne);
		STR(token_not);          STR(token_bnot);          STR(token_andsc);        STR(token_and);
		STR(token_orsc);         STR(token_or);            STR(token_eof);          STR(token_unknown);
	}
#undef STR
	return NULL;
}

void eat(enum token t)
{
	if(t != curtok)
		die_at("expecting token %s, got %s",
				token_to_str(t), token_to_str(curtok));
	nexttoken();
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
