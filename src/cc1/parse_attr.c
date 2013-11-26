static void parse_attr_bracket_chomp(int had_open_paren);

static decl_attr *parse_attr_format(void)
{
	/* __attribute__((format (printf, fmtarg, firstvararg))) */
	decl_attr *da;
	char *func;
	enum fmt_type fmt;

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

	da = decl_attr_new(attr_format);
	da->attr_extra.format.fmt_func = fmt;

	EAT(token_comma);

	da->attr_extra.format.fmt_arg = currentval.val - 1;
	EAT(token_integer);

	EAT(token_comma);

	da->attr_extra.format.var_arg = currentval.val - 1;
	EAT(token_integer);

	EAT(token_close_paren);

	return da;
}

static decl_attr *parse_attr_section()
{
	/* __attribute__((section ("sectionname"))) */
	decl_attr *da;
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

	da = decl_attr_new(attr_section);

	da->attr_extra.section = func;

	EAT(token_close_paren);

	return da;
}

static decl_attr *parse_attr_nonnull()
{
	/* __attribute__((nonnull(1, 2, 3, 4...)))
	 * or
	 * __attribute__((nonnull)) - all args
	 */
	decl_attr *da = decl_attr_new(attr_nonnull);
	unsigned long l = 0;
	int had_error = 0;

	if(accept(token_open_paren)){
		while(curtok != token_close_paren){
			if(curtok == token_integer){
				int n = currentval.val;
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
	da->attr_extra.nonnull_args = (l || had_error) ? l : ~0UL; /* all if 0 */

	return da;
}

static expr *optional_parened_expr(void)
{
	if(accept(token_open_paren)){
		expr *e;

		if(accept(token_close_paren))
			goto out;

		e = parse_expr_no_comma();

		EAT(token_close_paren);

		return e;
	}
out:
	return NULL;
}

static decl_attr *parse_attr_sentinel()
{
	decl_attr *da = decl_attr_new(attr_sentinel);

  da->attr_extra.sentinel = optional_parened_expr();

	return da;
}

static decl_attr *parse_attr_aligned()
{
	decl_attr *da = decl_attr_new(attr_aligned);

  da->attr_extra.align = optional_parened_expr();

	return da;
}

#define EMPTY(t)                      \
static decl_attr *parse_ ## t()       \
{                                     \
	return decl_attr_new(t);            \
}

EMPTY(attr_unused)
EMPTY(attr_warn_unused)
EMPTY(attr_enum_bitmask)
EMPTY(attr_noreturn)
EMPTY(attr_noderef)
EMPTY(attr_packed)

#undef EMPTY

static struct
{
	const char *ident;
	decl_attr *(*parser)(void);
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

static decl_attr *parse_attr_single(const char *ident)
{
	int i;

	for(i = 0; attrs[i].ident; i++){
		char buf[MAX_FMT_LEN];
		if(!strcmp(attrs[i].ident, ident)
		|| (snprintf(buf, sizeof buf, "__%s__", attrs[i].ident), !strcmp(buf, ident)))
		{
			return attrs[i].parser();
		}
	}

	warn_at(NULL, "ignoring unrecognised attribute \"%s\"", ident);

	/* if there are brackets, eat them all */

	parse_attr_bracket_chomp(0);

	return NULL;
}

static decl_attr *parse_attr(void)
{
	decl_attr *attr = NULL, **next = &attr;

	for(;;){
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

		EAT(curtok);

		if((*next = parse_attr_single(ident)))
			next = &(*next)->next;

		if(alloc)
			free(ident);

comma:
		if(!accept(token_comma))
			break;
	}

	return attr;
}
