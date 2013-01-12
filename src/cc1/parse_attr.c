decl_attr *parse_attr_format()
{
	/* __attribute__((format (printf, fmtarg, firstvararg))) */
	decl_attr *da;
	char *func;

	EAT(token_open_paren);

	func = token_current_spel();
	EAT(token_identifier);

	da = decl_attr_new(attr_format);

#define CHECK(s) !strcmp(func, s) || !strcmp(func, "__" s "__")

	if(CHECK("printf"))
		da->bits.format.fmt_func = attr_fmt_printf;
	else if(CHECK("scanf"))
		da->bits.format.fmt_func = attr_fmt_scanf;
	else
		DIE_AT(&da->where, "unknown format func \"%s\"", func);

	EAT(token_comma);

	da->bits.format.fmt_arg = currentval.val - 1;
	EAT(token_integer);

	EAT(token_comma);

	da->bits.format.var_arg = currentval.val - 1;
	EAT(token_integer);

	EAT(token_close_paren);

	return da;
}

decl_attr *parse_attr_section()
{
	/* __attribute__((section ("sectionname"))) */
	decl_attr *da;
	char *func;
	int len, i;

	EAT(token_open_paren);

	if(curtok != token_string)
		DIE_AT(NULL, "string expected for section");

	token_get_current_str(&func, &len);
	EAT(token_string);

	for(i = 0; i < len; i++)
		if(!isprint(func[i])){
			if(i < len - 1 || func[i] != '\0')
				warn_at(NULL, 1, "character 0x%x detected in section", func[i]);
			break;
		}

	da = decl_attr_new(attr_section);

	da->bits.section = func;

	EAT(token_close_paren);

	return da;
}

decl_attr *parse_attr_nonnull()
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
					WARN_AT(NULL, "%s nonnull argument ignored", n < 0 ? "negative" : "zero");
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

decl_attr *parse_attr_sentinel()
{
	decl_attr *da = decl_attr_new(attr_sentinel);

	if(accept(token_open_paren)){
		int u;

		EAT(token_integer);

		u = currentval.val;

		if(u < 0)
			WARN_AT(NULL, "negative sentinel argument ignored");
		else
			da->bits.sentinel = u;

		EAT(token_close_paren);
	}

	return da;
}

#define EMPTY(t)                      \
decl_attr *parse_ ## t()              \
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

#define CALL_CONV(n)                            \
decl_attr *parse_attr_## n()                    \
{                                               \
	decl_attr *a = decl_attr_new(attr_call_conv); \
	a->bits.conv = conv_ ## n;                    \
	return a;                                     \
}

CALL_CONV(cdecl)
CALL_CONV(stdcall)
CALL_CONV(fastcall)

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

	ATTR(cdecl),
	ATTR(stdcall),
	ATTR(fastcall),
#undef ATTR

	/* compat */
	{ "warn_unused_result", parse_attr_warn_unused },

	{ NULL, NULL },
};
#define MAX_FMT_LEN 16

void parse_attr_bracket_chomp(void)
{
	if(accept(token_open_paren)){
		parse_attr_bracket_chomp(); /* nest */

		while(curtok != token_close_paren)
			EAT(curtok);

		EAT(token_close_paren);
	}
}

decl_attr *parse_attr_single(char *ident)
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

	warn_at(NULL, 1, "ignoring unrecognised attribute \"%s\"", ident);

	/* if there are brackets, eat them all */

	parse_attr_bracket_chomp();

	return NULL;
}

decl_attr *parse_attr(void)
{
	decl_attr *attr = NULL, **next = &attr;

	for(;;){
		char *ident;

		if(curtok != token_identifier)
			DIE_AT(NULL, "identifier expected for attribute (got %s)", token_to_str(curtok));

		ident = token_current_spel();
		EAT(token_identifier);

		if((*next = parse_attr_single(ident)))
			next = &(*next)->next;

		free(ident);

		if(!accept(token_comma))
			break;
	}

	return attr;
}
