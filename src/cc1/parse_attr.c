decl_attr *parse_attr_format()
{
	/* __attribute__((__format__ (__printf__, fmtarg, firstvararg))) */
	decl_attr *da;
	char *func;

	EAT(token_open_paren);

	func = token_current_spel();
	EAT(token_identifier);

	da = decl_attr_new(attr_format);

	if(!strcmp(func, "__printf__"))
		da->attr_extra.format.fmt_func = attr_fmt_printf;
	else if(!strcmp(func, "__scanf__"))
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

#define EMPTY(t)                      \
decl_attr *parse_ ## t()              \
{                                     \
	return decl_attr_new(t);            \
}

EMPTY(attr_unused)
EMPTY(attr_warn_unused)

static struct
{
	const char *ident;
	decl_attr *(*parser)(void);
} attrs[] = {
	{ "__format__",         parse_attr_format },
	{ "__unused__",         parse_attr_unused },
	{ "__warn_unused__",    parse_attr_warn_unused },
	{ NULL, NULL },
};


decl_attr *parse_attr_single(char *ident)
{
	int i;

	for(i = 0; attrs[i].ident; i++)
		if(!strcmp(attrs[i].ident, ident))
			return attrs[i].parser();

	warn_at(NULL, "ignoring unrecognised attribute \"%s\"", ident);
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
