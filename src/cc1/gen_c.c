// TODO

void gen_asm(symtable *globs)
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
