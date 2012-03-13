#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "tokenise.h"
#include "tokconv.h"
#include "../util/util.h"
#include "macros.h"

extern enum token curtok;

enum type_primitive curtok_to_type_primitive()
{
	switch(curtok){
		case token_int:  return type_int;
		case token_char: return type_char;
		case token_void: return type_void;
		default: break;
	}
	return type_unknown;
}

enum type_spec curtok_to_type_specifier()
{
	switch(curtok){
		case token_const:    return spec_const;
		case token_extern:   return spec_extern;
		case token_static:   return spec_static;
		case token_signed:   return spec_signed;
		case token_unsigned: return spec_unsigned;
		case token_auto:     return spec_auto;
		case token_typedef:  return spec_typedef;
		default: break;
	}
	return spec_none;
}

int curtok_is_type()
{
	return curtok_to_type_primitive() != type_unknown;
}

int curtok_is_type_specifier()
{
	return curtok_to_type_specifier() != spec_none;
}

int curtok_is_augmented_assignment()
{
	switch(curtok){
		case token_plus:
		case token_minus:
		case token_multiply:
		case token_divide:
		case token_modulus:
		case token_not:
		case token_bnot:
		case token_and:
		case token_or:
		case token_xor:
		case token_shiftl:
		case token_shiftr:
			return 1;
		default:
			break;
	}
	return 0;
}

const char *token_to_str(enum token t)
{
	switch(t){
		CASE_STR_PREFIX(token,  do);               CASE_STR_PREFIX(token,  if);             CASE_STR_PREFIX(token,  else);            CASE_STR_PREFIX(token,  while);
		CASE_STR_PREFIX(token,  for);              CASE_STR_PREFIX(token,  break);          CASE_STR_PREFIX(token,  return);          CASE_STR_PREFIX(token,  switch);
		CASE_STR_PREFIX(token,  case);             CASE_STR_PREFIX(token,  default);        CASE_STR_PREFIX(token,  sizeof);          CASE_STR_PREFIX(token,  extern);
		CASE_STR_PREFIX(token,  identifier);       CASE_STR_PREFIX(token,  integer);        CASE_STR_PREFIX(token,  character);       CASE_STR_PREFIX(token,  void);
		CASE_STR_PREFIX(token,  char);             CASE_STR_PREFIX(token,  int);            CASE_STR_PREFIX(token,  elipsis);         CASE_STR_PREFIX(token,  string);
		CASE_STR_PREFIX(token,  open_paren);       CASE_STR_PREFIX(token,  open_block);     CASE_STR_PREFIX(token,  open_square);     CASE_STR_PREFIX(token,  close_paren);
		CASE_STR_PREFIX(token,  close_block);      CASE_STR_PREFIX(token,  close_square);   CASE_STR_PREFIX(token,  comma);           CASE_STR_PREFIX(token,  semicolon);
		CASE_STR_PREFIX(token,  colon);            CASE_STR_PREFIX(token,  plus);           CASE_STR_PREFIX(token,  minus);           CASE_STR_PREFIX(token,  multiply);
		CASE_STR_PREFIX(token,  divide);           CASE_STR_PREFIX(token,  modulus);        CASE_STR_PREFIX(token,  increment);       CASE_STR_PREFIX(token,  decrement);
		CASE_STR_PREFIX(token,  assign);           CASE_STR_PREFIX(token,  dot);            CASE_STR_PREFIX(token,  eq);              CASE_STR_PREFIX(token,  le);
		CASE_STR_PREFIX(token,  lt);               CASE_STR_PREFIX(token,  ge);             CASE_STR_PREFIX(token,  gt);              CASE_STR_PREFIX(token,  ne);
		CASE_STR_PREFIX(token,  not);              CASE_STR_PREFIX(token,  bnot);           CASE_STR_PREFIX(token,  andsc);           CASE_STR_PREFIX(token,  and);
		CASE_STR_PREFIX(token,  orsc);             CASE_STR_PREFIX(token,  or);             CASE_STR_PREFIX(token,  eof);             CASE_STR_PREFIX(token,  unknown);
		CASE_STR_PREFIX(token,  const);            CASE_STR_PREFIX(token,  question);       CASE_STR_PREFIX(token,  plus_assign);     CASE_STR_PREFIX(token,  minus_assign);
		CASE_STR_PREFIX(token,  multiply_assign);  CASE_STR_PREFIX(token,  divide_assign);  CASE_STR_PREFIX(token,  modulus_assign);  CASE_STR_PREFIX(token,  not_assign);
		CASE_STR_PREFIX(token,  bnot_assign);      CASE_STR_PREFIX(token,  and_assign);     CASE_STR_PREFIX(token,  or_assign);       CASE_STR_PREFIX(token,  static);
		CASE_STR_PREFIX(token,  xor);              CASE_STR_PREFIX(token,  xor_assign);     CASE_STR_PREFIX(token,  goto);            CASE_STR_PREFIX(token,  signed);
		CASE_STR_PREFIX(token,  unsigned);         CASE_STR_PREFIX(token,  auto);           CASE_STR_PREFIX(token,  shiftl);          CASE_STR_PREFIX(token,  shiftr);
		CASE_STR_PREFIX(token,  shiftl_assign);    CASE_STR_PREFIX(token,  shiftr_assign);  CASE_STR_PREFIX(token,  typedef);         CASE_STR_PREFIX(token,  struct);
		CASE_STR_PREFIX(token,  enum);             CASE_STR_PREFIX(token,  ptr);            CASE_STR_PREFIX(token,  continue);
	}
	return NULL;
}

void eat(enum token t, const char *fnam, int line)
{
	if(t != curtok){
		const int ident = curtok == token_identifier;
		die_at(NULL, "expecting token %s, got %s %s%s%s(%s:%d)",
				token_to_str(t), token_to_str(curtok),
				ident ? "\"" : "",
				ident ? token_current_spel_peek() : "",
				ident ? "\" " : "",
				fnam, line);
	}

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
	char *ret = cnam;               \
	cnam = NULL;                    \
	return ret;                     \
}

extern char *currentspelling;
NULL_AND_RET(token_current_spel, currentspelling)
char *token_current_spel_peek(void)
{
	return currentspelling;
}

void token_get_current_str(char **ps, int *pl)
{
	extern char *currentstring;
	extern int   currentstringlen;

	*ps = currentstring;
	*pl = currentstringlen;

	currentstring = NULL;
}
