decl_init *parse_initialisation(void)
{
	decl_init *di;

	if(accept(token_open_block)){
		struct decl_init_sub **subs;

		di = decl_init_new(decl_init_brace);
		subs = NULL;

		di = decl_init_new(decl_init_brace); /* subject to change */

		while(curtok != token_close_block){
			struct decl_init_sub *sub = umalloc(sizeof *sub);
			int struct_init = 0;
			char *ident;

			if(curtok == token_dot || curtok == token_open_square){
				if(curtok == token_open_square)
					ICE("TODO: { [0] = ... }");

				EAT(token_dot);

				ident = token_current_spel();
				EAT(token_identifier);

				if(curtok == token_dot)
					ICE("TODO: { .a.b = ... }");

				EAT(token_assign);
				struct_init = 1;
			}

			sub->init = parse_initialisation();

			if(struct_init)
				sub->spel = ident;

			dynarray_add((void ***)&subs, sub);

			if(!accept(token_comma))
				break;
		}

		di->bits.subs = subs;

		EAT(token_close_block);

	}else if(curtok == token_string){
		di = decl_init_new(decl_init_str);
		token_get_current_str(&di->bits.str.s, &di->bits.str.len);
		EAT(token_string);

	}else{
		di = decl_init_new(decl_init_scalar);
		di->bits.expr = parse_expr_no_comma();
	}

	return di;
}
