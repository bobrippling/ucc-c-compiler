#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../util/util.h"
#include "data_structs.h"
#include "tokenise.h"
#include "tokconv.h"
#include "../util/util.h"
#include "macros.h"
#include "cc1.h"

extern enum token curtok;

enum type_primitive curtok_to_type_primitive()
{
	switch(curtok){
		case token_void:  return type_void;

		case token__Bool: return type__Bool;
		case token_char:  return type_char;
		case token_short: return type_short;
		case token_int:   return type_int;
		case token_long:  return type_long;

		case token_float:  return type_float;
		case token_double: return type_double;

		default: break;
	}
	return type_unknown;
}

enum type_qualifier curtok_to_type_qualifier()
{
	switch(curtok){
		case token_const:    return qual_const;
		case token_volatile: return qual_volatile;
		case token_restrict: return qual_restrict;
		default:             return qual_none;
	}
}

enum type_storage curtok_to_type_storage()
{
	switch(curtok){
		case token_auto:     return store_auto;
		case token_extern:   return store_extern;
		case token_static:   return store_static;
		case token_typedef:  return store_typedef;
		case token_register: return store_register;
		default:             return -1;
	}
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

		case token_xor: return op_xor;
		case token_or: return op_or;
		case token_and: return op_and;
		case token_orsc: return op_orsc;
		case token_andsc: return op_andsc;
		case token_not: return op_not;
		case token_bnot: return op_bnot;

		case token_shiftl: return op_shiftl;
		case token_shiftr: return op_shiftr;

		case token_ptr: return op_struct_ptr;
		case token_dot: return op_struct_dot;

		default: break;
	}
	return op_unknown;
}

int curtok_is_type_primitive()
{
	return curtok_to_type_primitive() != type_unknown;
}

int curtok_is_type_qual()
{
	return curtok_to_type_qualifier() != qual_none;
}

int curtok_is_type_store()
{
	return curtok_to_type_storage() != (enum type_storage)-1;
}

enum op_type curtok_to_augmented_op()
{
#define CASE(x) case token_ ## x ## _assign: return op_ ## x
	switch(curtok){
		CASE(plus);
		CASE(minus);
		CASE(multiply);
		CASE(divide);
		CASE(modulus);
		CASE(not);
		CASE(bnot);
		CASE(and);
		CASE(or);
		CASE(xor);
		CASE(shiftl);
		CASE(shiftr);
		default:
			break;
	}
	return op_unknown;
#undef CASE
}

int curtok_is_augmented_assignment()
{
	return curtok_to_augmented_op() != op_unknown;
}

const char *token_to_str(enum token t)
{
	switch(t){
		CASE_STR_PREFIX(token,  do);
		CASE_STR_PREFIX(token,  if);
		CASE_STR_PREFIX(token,  else);
		CASE_STR_PREFIX(token,  while);
		CASE_STR_PREFIX(token,  for);
		CASE_STR_PREFIX(token,  break);
		CASE_STR_PREFIX(token,  return);
		CASE_STR_PREFIX(token,  switch);
		CASE_STR_PREFIX(token,  case);
		CASE_STR_PREFIX(token,  default);
		CASE_STR_PREFIX(token,  continue);
		CASE_STR_PREFIX(token,  goto);

		CASE_STR_PREFIX(token,  sizeof);
		CASE_STR_PREFIX(token,  typeof);

    CASE_STR_PREFIX(token,  _Generic);
    CASE_STR_PREFIX(token,  _Static_assert);

		/* storage */
		CASE_STR_PREFIX(token,  extern);
		CASE_STR_PREFIX(token,  static);
		CASE_STR_PREFIX(token,  auto);
		CASE_STR_PREFIX(token,  register);

		/* sort-of storage */
		CASE_STR_PREFIX(token,  inline);

		/* type-qual */
		CASE_STR_PREFIX(token,  const);
		CASE_STR_PREFIX(token,  volatile);
		CASE_STR_PREFIX(token,  restrict);

		/* type */
		CASE_STR_PREFIX(token,  void);
		CASE_STR_PREFIX(token,  char);
		CASE_STR_PREFIX(token,  short);
		CASE_STR_PREFIX(token,  int);
		CASE_STR_PREFIX(token,  long);
		CASE_STR_PREFIX(token,  float);
		CASE_STR_PREFIX(token,  double);
		CASE_STR_PREFIX(token,  _Bool);
		CASE_STR_PREFIX(token,  signed);
		CASE_STR_PREFIX(token,  unsigned);

		CASE_STR_PREFIX(token,  typedef);
		CASE_STR_PREFIX(token,  struct);
		CASE_STR_PREFIX(token,  union);
		CASE_STR_PREFIX(token,  enum);

		CASE_STR_PREFIX(token,  identifier);
		CASE_STR_PREFIX(token,  integer);
		CASE_STR_PREFIX(token,  character);
		CASE_STR_PREFIX(token,  elipsis);
		CASE_STR_PREFIX(token,  string);
		CASE_STR_PREFIX(token,  open_paren);
		CASE_STR_PREFIX(token,  open_block);
		CASE_STR_PREFIX(token,  open_square);
		CASE_STR_PREFIX(token,  close_paren);
		CASE_STR_PREFIX(token,  close_block);
		CASE_STR_PREFIX(token,  close_square);
		CASE_STR_PREFIX(token,  comma);
		CASE_STR_PREFIX(token,  semicolon);
		CASE_STR_PREFIX(token,  colon);
		CASE_STR_PREFIX(token,  plus);
		CASE_STR_PREFIX(token,  minus);
		CASE_STR_PREFIX(token,  multiply);
		CASE_STR_PREFIX(token,  divide);
		CASE_STR_PREFIX(token,  modulus);
		CASE_STR_PREFIX(token,  increment);
		CASE_STR_PREFIX(token,  decrement);
		CASE_STR_PREFIX(token,  assign);
		CASE_STR_PREFIX(token,  dot);
		CASE_STR_PREFIX(token,  eq);
		CASE_STR_PREFIX(token,  le);
		CASE_STR_PREFIX(token,  lt);
		CASE_STR_PREFIX(token,  ge);
		CASE_STR_PREFIX(token,  gt);
		CASE_STR_PREFIX(token,  ne);
		CASE_STR_PREFIX(token,  not);
		CASE_STR_PREFIX(token,  bnot);
		CASE_STR_PREFIX(token,  andsc);
		CASE_STR_PREFIX(token,  and);
		CASE_STR_PREFIX(token,  orsc);
		CASE_STR_PREFIX(token,  or);
		CASE_STR_PREFIX(token,  eof);
		CASE_STR_PREFIX(token,  unknown);
		CASE_STR_PREFIX(token,  question);
		CASE_STR_PREFIX(token,  plus_assign);
		CASE_STR_PREFIX(token,  minus_assign);
		CASE_STR_PREFIX(token,  multiply_assign);
		CASE_STR_PREFIX(token,  divide_assign);
		CASE_STR_PREFIX(token,  modulus_assign);
		CASE_STR_PREFIX(token,  not_assign);
		CASE_STR_PREFIX(token,  bnot_assign);
		CASE_STR_PREFIX(token,  and_assign);
		CASE_STR_PREFIX(token,  or_assign);
		CASE_STR_PREFIX(token,  xor);
		CASE_STR_PREFIX(token,  xor_assign);
		CASE_STR_PREFIX(token,  shiftl);
		CASE_STR_PREFIX(token,  shiftr);
		CASE_STR_PREFIX(token,  shiftl_assign);
		CASE_STR_PREFIX(token,  shiftr_assign);
		CASE_STR_PREFIX(token,  ptr);

		CASE_STR_PREFIX(token,  attribute);
	}
	return NULL;
}

void warn_at_print_error(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	vwarn(NULL, 1, 1, fmt, l);
	va_end(l);
}

void eat2(enum token t, const char *fnam, int line, int die)
{
	if(t != curtok){
		const int ident = curtok == token_identifier;
		parse_had_error = 1;

		warn_at_print_error(
				"expecting token %s, got %s %s%s%s(%s:%d)",
				token_to_str(t), token_to_str(curtok),
				ident ? "\"" : "",
				ident ? token_current_spel_peek() : "",
				ident ? "\" " : "",
				fnam, line);

		if(die || --cc1_max_errors <= 0)
			exit(1);

		/* XXX: we continue here, assuming we had the token anyway */
	}else{
		nexttoken();
	}
}

void eat(enum token t, const char *fnam, int line)
{
	eat2(t, fnam, line, 0);
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
