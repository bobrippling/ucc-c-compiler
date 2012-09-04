decl_init *parse_initialisation(void)
{
	decl_init *di;

	if(accept(token_open_block)){
		decl_init **exps = NULL;

		di = decl_init_new(decl_init_brace);

		while(curtok != token_close_block){
			decl_init *sub;
#ifdef DINIT_WITH_STRUCT
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
#endif

			sub = parse_initialisation();

#ifdef DINIT_WITH_STRUCT
			if(struct_init)
				sub->spel = ident;
#endif

			dynarray_add((void ***)&exps, sub);

			if(!accept(token_comma))
				break;
		}

		di->bits.inits = exps;

		EAT(token_close_block);

	}else{
		di = decl_init_new(decl_init_scalar);
		di->bits.expr = parse_expr_no_comma();
	}

	return di;
}
