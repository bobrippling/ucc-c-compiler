#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "tokenise.h"
#include "tokconv.h"
#include "../util/util.h"
#include "macros.h"
#include "cc1.h"
#include "cc1_where.h"

extern enum token curtok;
static enum token curtok_save = token_unknown;

enum type_primitive curtok_to_type_primitive()
{
	switch(curtok){
		case token_void:  return type_void;

		case token__Bool: return type__Bool;
		case token_char:  return type_nchar;
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

enum decl_storage curtok_to_decl_storage()
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

int curtok_is_decl_store()
{
	return curtok_to_decl_storage() != (enum decl_storage)-1;
}

enum op_type curtok_to_compound_op()
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

int curtok_is_compound_assignment()
{
	return curtok_to_compound_op() != op_unknown;
}

char *token_to_str(enum token t)
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
		CASE_STR_PREFIX(token,  asm);

		CASE_STR_PREFIX(token,  sizeof);
		CASE_STR_PREFIX(token,  typeof);

    CASE_STR_PREFIX(token,  _Generic);
    CASE_STR_PREFIX(token,  _Static_assert);

		/* storage */
		CASE_STR_PREFIX(token,  extern);
		CASE_STR_PREFIX(token,  static);
		CASE_STR_PREFIX(token,  auto);
		CASE_STR_PREFIX(token,  register);
		CASE_STR_PREFIX(token, _Alignof);
		CASE_STR_PREFIX(token, _Alignas);

		/* sort-of storage */
		CASE_STR_PREFIX(token,  inline);
		CASE_STR_PREFIX(token,  _Noreturn);

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
		CASE_STR_PREFIX(token,  __builtin_va_list);

		CASE_STR_PREFIX(token,  identifier);
		CASE_STR_PREFIX(token,  character);
		CASE_STR_PREFIX(token,  string);

		CASE_STR_PREFIX(token,  __extension__);
		CASE_STR_PREFIX(token,  __auto_type);

		case token_integer: return "integer-literal";
		case token_floater: return "float-literal";

#define MAP(t, s) case token_##t: return s
		MAP(attribute,       "__attribute__");
		MAP(elipsis,         "'...'");
		MAP(open_paren,      "'('");
		MAP(open_block,      "'{'");
		MAP(open_square,     "'['");
		MAP(close_paren,     "')'");
		MAP(close_block,     "'}'");
		MAP(close_square,    "']'");
		MAP(comma,           "','");
		MAP(semicolon,       "';'");
		MAP(colon,           "':'");
		MAP(plus,            "'+'");
		MAP(minus,           "'-'");
		MAP(multiply,        "'*'");
		MAP(divide,          "'/'");
		MAP(modulus,         "'%'");
		MAP(increment,       "'++'");
		MAP(decrement,       "'--'");
		MAP(assign,          "'='");
		MAP(dot,             "'.'");
		MAP(eq,              "'=='");
		MAP(le,              "'<='");
		MAP(lt,              "'<'");
		MAP(ge,              "'>='");
		MAP(gt,              "'>'");
		MAP(ne,              "'!='");
		MAP(not,             "'!'");
		MAP(bnot,            "'~'");
		MAP(andsc,           "'&&'");
		MAP(and,             "'&'");
		MAP(orsc,            "'||'");
		MAP(or,              "'|'");
		MAP(xor,             "'^'");
		MAP(question,        "'?'");
		MAP(plus_assign,     "'+='");
		MAP(minus_assign,    "'-='");
		MAP(multiply_assign, "'*='");
		MAP(divide_assign,   "'/='");
		MAP(modulus_assign,  "'%='");
		MAP(not_assign,      "'!='");
		MAP(bnot_assign,     "'~='");
		MAP(and_assign,      "'&='");
		MAP(or_assign,       "'|='");
		MAP(xor_assign,      "'^='");
		MAP(shiftl,          "'<<'");
		MAP(shiftr,          "'>>'");
		MAP(shiftl_assign,   "'<<='");
		MAP(shiftr_assign,   "'>>='");
		MAP(ptr,             "'->'");
#undef MAP

		case token_eof:             return  "eof";
		case token_unknown:         return  NULL;
	}
	return NULL;
}

char *curtok_to_identifier(int *alloc)
{
	switch(curtok){
		case token_do:
		case token_if:
		case token_else:
		case token_while:
		case token_for:
		case token_break:
		case token_return:
		case token_switch:
		case token_case:
		case token_default:
		case token_continue:
		case token_goto:
		case token_asm:
		case token_sizeof:
		case token_typeof:
		case token__Generic:
		case token__Static_assert:
		case token_extern:
		case token_static:
		case token_auto:
		case token_register:
		case token__Alignof:
		case token__Alignas:
		case token_inline:
		case token__Noreturn:
		case token_const:
		case token_volatile:
		case token_restrict:
		case token_void:
		case token_char:
		case token_short:
		case token_int:
		case token_long:
		case token_float:
		case token_double:
		case token__Bool:
		case token_signed:
		case token_unsigned:
		case token_typedef:
		case token_struct:
		case token_union:
		case token_enum:
		case token___builtin_va_list:
		case token_attribute:
		case token___extension__:
		case token___auto_type:
			/* we can stringify these */
			*alloc = 0;
			return token_to_str(curtok);
		case token_identifier:
			*alloc = 1;
			return token_current_spel();

		case token_integer:
		case token_floater:
		case token_character:
		case token_string:
		case token_elipsis:
		case token_open_paren:
		case token_open_block:
		case token_open_square:
		case token_close_paren:
		case token_close_block:
		case token_close_square:
		case token_comma:
		case token_semicolon:
		case token_colon:
		case token_plus:
		case token_minus:
		case token_multiply:
		case token_divide:
		case token_modulus:
		case token_increment:
		case token_decrement:
		case token_assign:
		case token_dot:
		case token_eq:
		case token_le:
		case token_lt:
		case token_ge:
		case token_gt:
		case token_ne:
		case token_not:
		case token_bnot:
		case token_andsc:
		case token_and:
		case token_orsc:
		case token_or:
		case token_xor:
		case token_question:
		case token_plus_assign:
		case token_minus_assign:
		case token_multiply_assign:
		case token_divide_assign:
		case token_modulus_assign:
		case token_not_assign:
		case token_bnot_assign:
		case token_and_assign:
		case token_or_assign:
		case token_xor_assign:
		case token_shiftl:
		case token_shiftr:
		case token_shiftl_assign:
		case token_shiftr_assign:
		case token_ptr:
		case token_eof:
		case token_unknown:
			break;
	}
	return NULL;
}

void eat2(enum token t, const char *fnam, int line, int die)
{
	if(t != curtok){
		const int ident = curtok == token_identifier;
		parse_had_error = 1;

		warn_at_print_error(NULL,
				"expecting token %s, got %s %s%s%s(%s:%d)",
				token_to_str(t), token_to_str(curtok),
				ident ? "\"" : "",
				ident ? token_current_spel_peek() : "",
				ident ? "\" " : "",
				fnam, line);

		if(die || --cc1_error_limit <= 0)
			exit(1);

		/* XXX: we continue here, assuming we had the token anyway */
	}else{
		if(curtok_save != token_unknown){
			curtok = curtok_save;
			curtok_save = token_unknown;
		}else{
			nexttoken();
		}
	}
}

int accept_where(enum token t, where *w)
{
	if(t == curtok){
		if(w)
			where_cc1_current(w);
		eat(t, NULL, 0); /* can't fail */
		return 1;
	}
	return 0;
}

int accept(enum token t)
{
	return accept_where(t, NULL);
}

void uneat(enum token t)
{
	UCC_ASSERT(curtok_save == token_unknown, "curtok regurgitate buffer full");

	/* if current is an identifier, abort,
	 * since we can't hold two in currentspelling */
	UCC_ASSERT(curtok_save == token_identifier ? t != token_identifier : 1,
			"can't save another identifier");

	curtok_save = curtok;
	curtok = t;
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

void token_get_current_str(
		char **ps, size_t *pl, int *pwide, where *w)
{
	extern char *currentstring;
	extern size_t currentstringlen;
	extern int   currentstringwide;

	*ps = currentstring;

	if(pwide)
		*pwide = currentstringwide;
	else if(currentstringwide)
		die_at(NULL, "wide string not wanted");

	if(w){
		extern where currentstringwhere;
		memcpy_safe(w, &currentstringwhere);
	}

	if(pl){
		*pl = currentstringlen;
	}else{
		char *p = memchr(currentstring, '\0', currentstringlen);

		if(p && p < currentstring + currentstringlen - 1)
			cc1_warn_at(NULL, str_contain_nul,
					"nul-character terminates string early (%s)", p + 1);
	}

	currentstring = NULL;
	currentstringlen = 0;
}
