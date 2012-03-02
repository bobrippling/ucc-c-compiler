#include <stdlib.h>

#include "ops.h"
#include "stat_code.h"

const char *str_stat_code()
{
	return "code";
}

void fold_stat_code(stat *s)
{
	decl **iter;

	fold_symtab_scope(s->symtab);

	for(iter = s->decls; iter && *iter; iter++){
		decl *d = *iter;

		if(d->func_code)
			die_at(&d->func_code->where, "can's nest functions");

		fold_decl(d, s->symtab);

		SYMTAB_ADD(s->symtab, d, sym_local);
	}

	if(s->codes){
		stat **iter;
		for(iter = s->codes; *iter; iter++)
			fold_stat(*iter);
	}

	/* static folding */
	if(s->decls){
		decl **iter;

		for(iter = s->decls; *iter; iter++){
			decl *d = *iter;
			/*
				* check static decls - after we fold,
				* so we've linked the syms and can change ->spel
				*/
			if(d->type->spec & spec_static){
				char *save = d->spel;
				d->spel = asm_label_static_local(curdecl_func_sp, d->spel);
				free(save);
			}
		}
	}
}

void gen_stat_code(stat *s)
{
	if(s->codes){
		stat **titer;
		decl **diter;

		/* declare statics */
		for(diter = s->decls; diter && *diter; diter++){
			decl *d = *diter;
			if(d->type->spec & (spec_static | spec_extern))
				gen_asm_global(d);
		}

		for(titer = s->codes; *titer; titer++)
			gen_stat(*titer);
	}
}
