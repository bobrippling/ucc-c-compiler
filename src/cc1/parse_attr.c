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
		da->attr_extra.format.fmt_func = attr_fmt_printf;
	else if(CHECK("scanf"))
		da->attr_extra.format.fmt_func = attr_fmt_scanf;
	else
		die_at(&da->where, "unknown format func \"%s\"", func);

	EAT(token_comma);

	da->attr_extra.format.fmt_arg = currentval.val;
	EAT(token_integer);

	EAT(token_comma);

	da->attr_extra.format.var_arg = currentval.val;
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
		die_at(NULL, "string expected for section");

	token_get_current_str(&func, &len);
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

#define EMPTY(t)                      \
decl_attr *parse_ ## t()              \
{                                     \
	return decl_attr_new(t);            \
}

EMPTY(attr_unused)
EMPTY(attr_warn_unused)
EMPTY(attr_overloadable)

static struct
{
	const char *ident;
	decl_attr *(*parser)(void);
} attrs[] = {
	{ "format",         parse_attr_format },
	{ "unused",         parse_attr_unused },
	{ "warn_unused",    parse_attr_warn_unused },
	{ "section",        parse_attr_section },
	{ "overloadable",   parse_attr_overloadable },
#define MAX_FMT_LEN 32
	{ NULL, NULL },
};

void parse_attr_bracket_chomp(void)
{
	if(accept(token_open_paren)){
		parse_attr_bracket_chomp(); /* nest */

		accept(token_comma);
		accept(token_identifier); /* optional */

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

	warn_at(NULL, "ignoring unrecognised attribute \"%s\"", ident);

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
			die_at(NULL, "identifier expected for attribute");

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
