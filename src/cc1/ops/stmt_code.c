#include <stdlib.h>

#include "ops.h"
#include "stmt_code.h"

const char *str_stmt_code()
{
	return "code";
}

void fold_stmt_code(stmt *s)
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
		stmt **iter;
		for(iter = s->codes; *iter; iter++)
			fold_stmt(*iter);
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
				d->spel = asm_label_stmtic_local(curdecl_func_sp, d->spel);
				free(save);
			}
		}
	}
}

void gen_stmt_code(stmt *s)
{
	if(s->codes){
		stmt **titer;
		decl **diter;

		/* declare stmtics */
		for(diter = s->decls; diter && *diter; diter++){
			decl *d = *diter;
			if(d->type->spec & (spec_static | spec_extern))
				gen_asm_global(d);
		}

		for(titer = s->codes; *titer; titer++)
			gen_stmt(*titer);
	}
}
