#define INT_TYPE(t) t = type_new(); t->primitive = type_int
type *parse_type_struct()
{
	char *spel;
	type *t;
	struc *struc;

	t = NULL;

	if(curtok == token_identifier){
		spel = token_current_spel();
		EAT(token_identifier);
	}else{
		spel = NULL;
	}

	if(accept(token_open_block)){
		struc = umalloc(sizeof *struc);

		struc->spel = spel;
		struc->members = parse_decls(DECL_CAN_DEFAULT | DECL_SPEL_NEED);

		EAT(token_close_block);

		dynarray_add((void ***)&structs_current, struc);
	}else{
		struc = NULL;
	}

	t = type_new();
	t->struc = struc;

	return t;
}

type *parse_type()
{
	enum type_spec spec = 0;
	type *t = NULL;
	decl *td;
	int flag;

	while((flag = curtok_is_type_specifier()) || curtok_is_type() || (td = TYPEDEF_FIND())){
		if(flag){
			const enum type_spec this = curtok_to_type_specifier();

			/* we can't check in fold, since 1 & 1 & 1 is still just 1 */
			if(spec & spec)
				die_at(NULL, "duplicate type specifier \"%s\"", spec_to_str(spec));

			spec |= this;
			EAT(curtok);
		}else if(td){
			/* typedef name */

			if(t)
				break; /* "int x" - we are at x */

			td = TYPEDEF_FIND();
			if(!td)
				return NULL;
			warn_at(NULL, "yo");
			ICE("typedef todo");
		}else if(t){
			die_at(NULL, "type name unexpected");
		}else{
			t = type_new();
			t->primitive = curtok_to_type_primitive();
			EAT(curtok);
		}
	}

	if(!t && spec){
		/* unsigned x; */
		INT_TYPE(t);
	}

	if(t)
		t->spec = spec;

	return t;
}

decl *parse_decl(type *t, enum decl_mode mode)
{
	decl *d = decl_new();

	d->type = t;

	while(accept(token_multiply))
		/* FIXME: int *const x; */
		d->ptr_depth++;

	if(curtok == token_identifier){
		if(mode & DECL_SPEL_NO)
			die_at(&d->where, "identifier unexpected");

		d->spel = token_current_spel();
		EAT(token_identifier);
	}else if(mode & DECL_SPEL_NEED){
		EAT(token_identifier);
	}

	if(accept(token_open_paren))
		d->func = parse_function();
	else if(accept(token_assign))
		d->init = parse_expr();

	return d;
}

decl *parse_decl_single(enum decl_mode mode)
{
	type *t = parse_type();

	if(!t){
		if(mode & DECL_CAN_DEFAULT){
			INT_TYPE(t);
			cc1_warn_at(&t->where, 0, WARN_IMPLICIT_INT, "defaulting type to int");
		}else{
			return NULL;
		}
	}

	return parse_decl(t, mode);
}

decl **parse_decls(const int can_default)
{
	const enum decl_mode flag = DECL_SPEL_NEED | (can_default ? DECL_CAN_DEFAULT : 0);
	decl **ret = NULL;
	decl *last;

	/* read a type, then *spels separated by commas, then a semi colon, then repeat */
	for(;;){
		decl *d;
		type *t;

		last = NULL;

		t = parse_type();

		if(!t){
			if(can_default){
				INT_TYPE(t);
				cc1_warn_at(&t->where, 0, WARN_IMPLICIT_INT, "defaulting type to int");
			}else{
				return ret;
			}
		}

		do{
			d = parse_decl(t, flag);
			if(d){
				dynarray_add((void ***)&ret, d);
				if(d->func && d->func->code){
					if(curtok == token_eof)
						return ret;
					continue;
				}

				last = d;
			}else{
				break;
			}
		}while(accept(token_comma));

		if(last && !last->func)
			EAT(token_semicolon);
	}
}
