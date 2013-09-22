// TODO

basic_blk *gen_asm(symtable *globs, basic_blk *bb)
{
	decl **diter;
	for(diter = globs->decls; diter && *diter; diter++){
		decl *d = *diter;

		if(d->func)
			gen_c_func(d);
		else
			gen_c_global_var(d);
	}
}
