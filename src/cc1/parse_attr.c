#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"

#include "parse_attr.h"

#include "tokenise.h"
#include "tokconv.h"

#include "cc1_where.h"

#include "parse_expr.h"

static void parse_attr_bracket_chomp(int had_open_paren);

static attribute *parse_attr_format(symtable *scope)
{
	/* __attribute__((format (printf, fmtarg, firstvararg))) */
	attribute *da;
	char *func;
	enum fmt_type fmt;

	(void)scope;

	EAT(token_open_paren);

	func = token_current_spel();
	EAT(token_identifier);

	/* TODO: token_current_spel()
	 * and token_get_current_str(..,..)
	 * checks everywhere */
	if(!func)
		return NULL;

#define CHECK(s) !strcmp(func, s) || !strcmp(func, "__" s "__")
	if(CHECK("printf")){
		fmt = attr_fmt_printf;
	}else if(CHECK("scanf")){
		fmt = attr_fmt_scanf;
	}else{
		warn_at(NULL, "unknown format func \"%s\"", func);
		parse_attr_bracket_chomp(1);
		return NULL;
	}

	da = attribute_new(attr_format);
	da->bits.format.fmt_func = fmt;

	EAT(token_comma);

	da->bits.format.fmt_idx = currentval.val.i - 1;
	EAT(token_integer);

	EAT(token_comma);

	da->bits.format.var_idx = currentval.val.i - 1;
	EAT(token_integer);

	EAT(token_close_paren);

	return da;
}

static attribute *parse_attr_section()
{
	/* __attribute__((section ("sectionname"))) */
	attribute *da;
	char *func;
	size_t len, i;

	EAT(token_open_paren);

	if(curtok != token_string)
		die_at(NULL, "string expected for section");

	token_get_current_str(&func, &len, NULL, NULL);
	EAT(token_string);

	for(i = 0; i < len; i++)
		if(!isprint(func[i])){
			if(i < len - 1 || func[i] != '\0')
				warn_at(NULL, "character 0x%x detected in section", func[i]);
			break;
		}

	da = attribute_new(attr_section);

	da->bits.section = func;

	EAT(token_close_paren);

	return da;
}

static attribute *parse_attr_nonnull()
{
	/* __attribute__((nonnull(1, 2, 3, 4...)))
	 * or
	 * __attribute__((nonnull)) - all args
	 */
	attribute *da = attribute_new(attr_nonnull);
	unsigned long l = 0;
	int had_error = 0;

	if(accept(token_open_paren)){
		while(curtok != token_close_paren){
			if(curtok == token_integer){
				int n = currentval.val.i;
				if(n <= 0){
					/* shouldn't ever be negative */
					warn_at(NULL, "%s nonnull argument ignored", n < 0 ? "negative" : "zero");
					had_error = 1;
				}else{
					/* implicitly disallow functions with >32 args */
					/* n-1, since we convert from 1-base to 0-base */
					l |= 1 << (n - 1);
				}
			}else{
				EAT(token_integer); /* raise error */
			}
			EAT(curtok);

			if(accept(token_comma))
				continue;
			break;
		}
		EAT(token_close_paren);
	}

	/* if we had an error, go with what we've got, (even if it's nothing), to avoid spurious warnings */
	da->bits.nonnull_args = (l || had_error) ? l : ~0UL; /* all if 0 */

	return da;
}

static expr *optional_parened_expr(symtable *scope)
{
	if(accept(token_open_paren)){
		expr *e;

		if(accept(token_close_paren))
			goto out;

		e = PARSE_EXPR_NO_COMMA(scope);

		EAT(token_close_paren);

		return e;
	}
out:
	return NULL;
}

static attribute *parse_attr_sentinel(symtable *scope)
{
	attribute *da = attribute_new(attr_sentinel);

  da->bits.sentinel = optional_parened_expr(scope);

	return da;
}

static attribute *parse_attr_aligned(symtable *scope)
{
	attribute *da = attribute_new(attr_aligned);

  da->bits.align = optional_parened_expr(scope);

	return da;
}

#define EMPTY(t)                      \
static attribute *parse_ ## t()       \
{                                     \
	return attribute_new(t);            \
}

EMPTY(attr_unused)
EMPTY(attr_warn_unused)
EMPTY(attr_enum_bitmask)
EMPTY(attr_noreturn)
EMPTY(attr_noderef)
EMPTY(attr_packed)
EMPTY(attr_ucc_debug)

#undef EMPTY

#define CALL_CONV(n)                            \
static attribute *parse_attr_## n()             \
{                                               \
	attribute *a = attribute_new(attr_call_conv); \
	a->bits.conv = conv_ ## n;                    \
	return a;                                     \
}

CALL_CONV(cdecl)
CALL_CONV(stdcall)
CALL_CONV(fastcall)

static struct
{
	const char *ident;
	attribute *(*parser)(symtable *);
} attrs[] = {
#define ATTR(x) { #x, parse_attr_ ## x }
	ATTR(format),
	ATTR(unused),
	ATTR(warn_unused),
	ATTR(section),
	ATTR(enum_bitmask),
	ATTR(noreturn),
	ATTR(noderef),
	ATTR(nonnull),
	ATTR(packed),
	ATTR(sentinel),
	ATTR(aligned),
	{ "__ucc_debug", parse_attr_ucc_debug },

	ATTR(cdecl),
	ATTR(stdcall),
	ATTR(fastcall),
#undef ATTR

	/* compat */
	{ "warn_unused_result", parse_attr_warn_unused },

	{ NULL, NULL },
};
#define MAX_FMT_LEN 16

static void parse_attr_bracket_chomp(int had_open_paren)
{
	if(!had_open_paren && accept(token_open_paren))
		had_open_paren = 1;

	if(had_open_paren){
		parse_attr_bracket_chomp(0); /* nest */

		while(curtok != token_close_paren)
			EAT(curtok);

		EAT(token_close_paren);
	}
}

static attribute *parse_attr_single(const char *ident, symtable *scope)
{
	int i;

	for(i = 0; attrs[i].ident; i++){
		char buf[MAX_FMT_LEN];
		if(!strcmp(attrs[i].ident, ident)
		|| (snprintf(buf, sizeof buf, "__%s__", attrs[i].ident), !strcmp(buf, ident)))
		{
			return attrs[i].parser(scope);
		}
	}

	warn_at(NULL, "ignoring unrecognised attribute \"%s\"", ident);

	/* if there are brackets, eat them all */

	parse_attr_bracket_chomp(0);

	return NULL;
}

attribute *parse_attr(symtable *scope)
{
	attribute *attr = NULL, **next = &attr;

	for(;;){
		attribute *this;
		where w;
		int alloc;
		char *ident = curtok_to_identifier(&alloc);

		if(!ident){
			parse_had_error = 1;
			warn_at_print_error(NULL,
					"identifier expected for attribute (got %s)",
					token_to_str(curtok));
			EAT(curtok);
			goto comma;
		}

		where_cc1_current(&w);
		where_cc1_adj_identifier(&w, ident);

		EAT(curtok);

		if((this = *next = parse_attr_single(ident, scope))){
			memcpy_safe(&this->where, &w);
			next = &(*next)->next;
		}

		if(alloc)
			free(ident);

comma:
		if(!accept(token_comma))
			break;
	}

	return attr;
}
