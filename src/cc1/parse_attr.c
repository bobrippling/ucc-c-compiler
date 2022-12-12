#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../util/util.h"
#include "../util/alloc.h"
#include "../util/dynarray.h"
#include "../util/str.h"

#include "parse_attr.h"

#include "tokenise.h"
#include "tokconv.h"
#include "str.h"

#include "fold.h"

#include "cc1_where.h"
#include "warn.h"

#include "fold.h"

#include "parse_expr.h"
#include "cc1_target.h"

static void parse_attr_bracket_chomp(int had_open_paren);

static attribute *parse_attr_format(symtable *symtab, const char *ident)
{
	/* __attribute__((format (printf, fmtarg, firstvararg))) */
	attribute *da;
	char *func;
	enum fmt_type fmt;

	(void)symtab;
	(void)ident;

	EAT(token_open_paren);

	func = token_eat_identifier(DUMMY_IDENTIFIER, NULL);

#define CHECK(s) !strcmp(func, s) || !strcmp(func, "__" s "__")
	if(CHECK("printf")){
		fmt = attr_fmt_printf;
	}else if(CHECK("scanf")){
		fmt = attr_fmt_scanf;
	}else{
		cc1_warn_at(NULL, attr_format_unknown,
				"unknown format func \"%s\"", func);
		parse_attr_bracket_chomp(1);
		return NULL;
	}

	da = attribute_new(attr_format);
	da->bits.format.fmt_func = fmt;

	EAT(token_comma);

	da->bits.format.fmt_idx = PARSE_EXPR_NO_COMMA(symtab, 1);
	FOLD_EXPR(da->bits.format.fmt_idx, symtab);

	EAT(token_comma);

	da->bits.format.var_idx = PARSE_EXPR_NO_COMMA(symtab, 1);
	FOLD_EXPR(da->bits.format.var_idx, symtab);

	EAT(token_close_paren);

	return da;
}

static attribute *parse_attr_section(symtable *symtab, const char *ident)
{
	/* __attribute__((section ("sectionname"))) */
	attribute *da;
	struct cstring *str;
	char *func;
	size_t len, i;

	(void)symtab;
	(void)ident;

	EAT(token_open_paren);

	if(curtok != token_string)
		die_at(NULL, "string expected for section");

	str = parse_asciz_str();
	EAT(token_string);
	if(!str)
		return NULL;
	len = str->count;
	func = cstring_detach(str);

	for(i = 0; i < len; i++)
		if(!isprint(func[i])){
			if(i < len - 1 || func[i] != '\0')
				cc1_warn_at(NULL, attr_section_badchar,
						"character 0x%x detected in section", func[i]);
			break;
		}

	da = attribute_new(attr_section);

	da->bits.section = func;

	EAT(token_close_paren);

	return da;
}

static attribute *parse_attr_nonnull(symtable *symtab, const char *ident)
{
	/* __attribute__((nonnull(1, 2, 3, 4...)))
	 * or
	 * __attribute__((nonnull)) - all args
	 */
	attribute *da = attribute_new(attr_nonnull);
	unsigned long l = 0;
	int had_error = 0;

	(void)symtab;
	(void)ident;

	if(accept(token_open_paren)){
		while(curtok != token_close_paren){
			expr *e = PARSE_EXPR_NO_COMMA(symtab, 1);
			consty k;

			FOLD_EXPR(e, symtab);
			const_fold(e, &k);

			if(k.type != CONST_NUM || k.bits.num.suffix & VAL_FLOATING){
				cc1_warn_at(NULL,
						attr_nonnull_bad,
						"nonnull argument not an integer constant");
				had_error = 1;
			}else{
				integral_t n = k.bits.num.val.i;
				if(n <= 0){
					/* shouldn't ever be negative */
					cc1_warn_at(NULL,
							attr_nonnull_bad,
							"%s nonnull argument ignored", (sintegral_t)n < 0 ? "negative" : "zero");
					had_error = 1;
				}else{
					/* implicitly disallow functions with >32 args */
					/* n-1, since we convert from 1-base to 0-base */
					if(n < sizeof(l) * CHAR_BIT)
						l |= 1 << (n - 1);
				}
			}

			expr_free(e);

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

		e = PARSE_EXPR_NO_COMMA(scope, 0);
		FOLD_EXPR(e, scope);

		EAT(token_close_paren);

		return e;
	}
out:
	return NULL;
}

static attribute *parse_attr_sentinel(symtable *scope, const char *ident)
{
	attribute *da = attribute_new(attr_sentinel);

	(void)ident;

  da->bits.sentinel = optional_parened_expr(scope);

	return da;
}

static attribute *parse_attr_aligned(symtable *scope, const char *ident)
{
	attribute *da = attribute_new(attr_aligned);

	(void)ident;

  da->bits.align = optional_parened_expr(scope);

	return da;
}

static attribute *parse_attr_cleanup(symtable *scope, const char *ident)
{
	char *sp;
	where ident_loc;
	attribute *attr = NULL;
	struct symtab_entry ent;

	(void)ident;

	EAT(token_open_paren);

	if(curtok != token_identifier)
		die_at(NULL, "identifier expected for cleanup function");

	sp = token_eat_identifier(DUMMY_IDENTIFIER, &ident_loc);

	if(symtab_search(scope, sp, NULL, &ent) && ent.type == SYMTAB_ENT_DECL){
		attr = attribute_new(attr_cleanup);
		attr->bits.cleanup = ent.bits.decl;
		decl_use(ent.bits.decl);
	}else{
		warn_at_print_error(&ident_loc, "function '%s' not found", sp);
		fold_had_error = 1;
	}

	EAT(token_close_paren);

	return attr;
}

static attribute *parse_attr_ctor_dtor(
		enum attribute_type ty, symtable *scope)
{
	attribute *ctor = attribute_new(ty);
	ctor->bits.priority = optional_parened_expr(scope);
	return ctor;
}

static attribute *parse_attr_constructor(symtable *scope, const char *ident)
{
	(void)ident;
	return parse_attr_ctor_dtor(attr_constructor, scope);
}

static attribute *parse_attr_destructor(symtable *scope, const char *ident)
{
	(void)ident;
	return parse_attr_ctor_dtor(attr_destructor, scope);
}

#define EMPTY(t)                                \
static attribute *parse_ ## t(                  \
		symtable *symtab, const char *ident)        \
{                                               \
	(void)symtab;                                 \
	(void)ident;                                  \
	return attribute_new(t);                      \
}

EMPTY(attr_unused)
EMPTY(attr_used)
EMPTY(attr_warn_unused)
EMPTY(attr_enum_bitmask)
EMPTY(attr_noreturn)
EMPTY(attr_noderef)
EMPTY(attr_packed)
EMPTY(attr_weak)
EMPTY(attr_always_inline)
EMPTY(attr_noinline)
EMPTY(attr_ucc_debug)
EMPTY(attr_desig_init)
EMPTY(attr_stack_protect)
EMPTY(attr_no_stack_protector)
EMPTY(attr_returns_nonnull)
EMPTY(attr_fallthrough)
EMPTY(attr_flatten)

#undef EMPTY

static attribute *parse_attr_call_conv(symtable *symtab, const char *ident)
{
	attribute *a = attribute_new(attr_call_conv);

	(void)symtab;

	/**/ if(!strcmp(ident, "cdecl"))
		a->bits.conv = conv_cdecl;
	else if(!strcmp(ident, "stdcall"))
		a->bits.conv = conv_stdcall;
	else if(!strcmp(ident, "fastcall"))
		a->bits.conv = conv_fastcall;
	else
		assert(0 && "unreachable");

	return a;
}

static char *parse_single_string_attr(const char *desc, where *str_loc)
{
	struct cstring *asciz;
	char *str = NULL;

	EAT(token_open_paren);

	if(curtok != token_string){
		warn_at_print_error(NULL, "string expected for %s", desc);
		parse_had_error = 1;
		return str;
	}

	where_cc1_current(str_loc);
	asciz = parse_asciz_str();

	EAT(token_string);

	if(asciz)
		str = cstring_detach(asciz);

	EAT(token_close_paren);

	return str;
}

static attribute *parse_attr_visibility(symtable *symtab, const char *ident)
{
	where str_loc;
	char *str = parse_single_string_attr("visibility", &str_loc);
	attribute *attr = NULL;
	enum visibility v;

	(void)symtab;
	(void)ident;

	if(!str)
		return NULL;

	if(visibility_parse(&v, str, cc1_target_details.as->supports_visibility_protected)){
		attr = attribute_new(attr_visibility);
		attr->bits.visibility = v;
	}else{
		warn_at_print_error(&str_loc, "unknown/unsupported visibility \"%s\"", str);
		fold_had_error = 1;
	}

	free(str);

	return attr;
}

static attribute *parse_attr_alias(symtable *scope, const char *ident)
{
	where str_loc;
	char *str = parse_single_string_attr("alias", &str_loc);
	attribute *attr = NULL;
	struct symtab_entry ent;

	(void)ident;

	if(!str)
		return NULL;

	if(symtab_search(scope, str, NULL, &ent)){
		if(ent.type == SYMTAB_ENT_DECL){
			attr = attribute_new(attr_alias);
			attr->bits.alias = ent.bits.decl;
		}else{
			warn_at_print_error(&str_loc, "alias \"%s\" references an enum member", str);
			fold_had_error = 1;
		}
	}else{
		warn_at_print_error(&str_loc, "alias \"%s\" doesn't exist", str);
		fold_had_error = 1;
	}

	free(str);

	return attr;
}

static attribute *parse_attr_no_sanitize(symtable *scope, const char *ident)
{
	attribute *attr = NULL;
	enum no_sanitize opts = 0;

	(void)scope;

	if(!strcmp(ident, "no_sanitize_undefined")){
		opts = NO_SANITIZE_UNDEFINED;
	}else{
		EAT(token_open_paren);

		while(curtok == token_string){
			where str_loc;
			struct cstring *asciz;
			char *str;

			where_cc1_current(&str_loc);
			asciz = parse_asciz_str();

			EAT(token_string);

			str = asciz ? cstring_detach(asciz) : NULL;

			if(str){
				char *tok, *state;

				for(tok = str_split(str, ',', &state); tok; tok = str_split(NULL, ',', &state)){
					if(!strcmp(tok, "undefined"))
						opts |= NO_SANITIZE_UNDEFINED;
					else
						cc1_warn_at(&str_loc, attr_unknown_sanitizer, "unknown sanitizer \"%s\"", tok);
				}

				free(str);
			}

			if(!accept(token_comma))
				break;
		}

		EAT(token_close_paren);
	}

	if(opts){
		attr = attribute_new(attr_no_sanitize);
		attr->bits.no_sanitize = opts;
	}

	return attr;
}

static const struct
{
	const char *ident;
	attribute *(*parser)(symtable *, const char *ident);
} attrs[] = {
#define NAME(x, typrop, tymismatch) { #x, parse_attr_ ## x },
#define RENAME(s, x, typrop, tymismatch) { s, parse_attr_ ## x },
#define ALIAS(s, x) { s, parse_attr_ ## x },
#define COMPLEX_ALIAS(s, x) { s, parse_attr_ ## x},
	ATTRIBUTES
#undef NAME
#undef ALIAS
#undef COMPLEX_ALIAS

	{ NULL, NULL },
};
#define MAX_FMT_LEN 16

static void parse_attr_bracket_chomp(int had_open_paren)
{
	if(had_open_paren || accept(token_open_paren)){
		for(;;){
			if(accept(token_open_paren))
				parse_attr_bracket_chomp(1); /* nest */

			if(accept(token_close_paren))
				break;
			else if(curtok == token_eof)
				break; /* failsafe */

			EAT(curtok);
		}
	}
}

static attribute *parse_attr_single(const char *ident, symtable *scope)
{
	symtable_global *glob;
	int i;
	where attrloc;

	for(i = 0; attrs[i].ident; i++){
		char buf[MAX_FMT_LEN];
		if(!strcmp(attrs[i].ident, ident)
		|| (snprintf(buf, sizeof buf, "__%s__", attrs[i].ident), !strcmp(buf, ident)))
		{
			return attrs[i].parser(scope, attrs[i].ident);
		}
	}

	where_cc1_current(&attrloc);
	attrloc.chr -= strlen(ident);

	/* unrecognised - only do the warning (and map checking) if non system-header */
	if(cc1_warning.system_headers || !where_in_sysheader(&attrloc)){
		glob = symtab_global(scope);
		if(!dynmap_exists(char *, glob->unrecog_attrs, (char *)ident)){
			char *dup = ustrdup(ident);

			if(!glob->unrecog_attrs)
				glob->unrecog_attrs = dynmap_new(char *, strcmp, dynmap_strhash);

			(void)dynmap_set(char *, void *, glob->unrecog_attrs, dup, NULL);

			cc1_warn_at(&attrloc, attr_unknown,
					"ignoring unrecognised attribute \"%s\"", ident);
		}
	}

	/* if there are brackets, eat them all */
	parse_attr_bracket_chomp(0);

	return NULL;
}

attribute **parse_attr(symtable *scope)
{
	attribute **attr = NULL;

	for(;;){
		attribute *this;
		where w;
		int alloc;
		char *ident;

		if(accept(token_comma))
			continue;
		if(curtok == token_close_paren)
			break;

		ident = eat_curtok_to_identifier(&alloc, &w);

		if(!ident){
			parse_had_error = 1;
			warn_at_print_error(NULL,
					"identifier expected for attribute (got %s)",
					token_to_str(curtok));
			EAT(curtok);
			goto comma;
		}

		this = parse_attr_single(ident, scope);

		if(this){
			memcpy_safe(&this->where, &w);
			dynarray_add(&attr, this);
		}

		if(alloc)
			free(ident);

comma:
		if(!accept(token_comma))
			break;
	}

	return attr;
}
