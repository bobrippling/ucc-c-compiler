#include <stdlib.h>
#include <string.h>

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
			DIE_AT(&d->func_code->where, "can't nest functions");

		fold_decl(d, s->symtab);
		d->is_definition = 1; /* always the def for non-globals */

		SYMTAB_ADD(s->symtab, d, sym_local);
	}

	if(s->codes){
		stmt **iter;
		int warned = 0;

		for(iter = s->codes; *iter; iter++){
			stmt  *const st = *iter;
			where *const old_w = eof_where;

			eof_where = &st->where;
			fold_stmt(st);
			eof_where = old_w;

			/*
			 * check for dead code
			 */
			if(!warned
			&& st->kills_below_code
			&& iter[1]
			&& !stmt_kind(iter[1], label)
			&& !stmt_kind(iter[1], case)
			&& !stmt_kind(iter[1], default)
			){
				cc1_warn_at(&iter[1]->where, 0, 1, WARN_DEAD_CODE, "dead code after %s (%s)", st->f_str(), iter[1]->f_str());
				warned = 1;
			}
		}
	}
}

void gen_code_decls(symtable *stab)
{
	decl **diter;

	/* declare statics */
	for(diter = stab->decls; diter && *diter; diter++){
		decl *d = *diter;
		int func;

		if((func = decl_is_func(d)) || type_store_static_or_extern(d->type->store)){
			int gen = 1;

			if(func){
				/* check if the func is defined globally */
				symtable *globs = symtab_root(stab);
				decl **i;

				for(i = globs->decls; i && *i; i++){
					if(!strcmp(d->spel_asm, (*i)->spel)){
						gen = 0;
						break;
					}
				}
			}

			if(gen){
				d->is_definition = 1;
				gen_asm_global(d);
			}

			fprintf(stderr, "DECL %s (%s) gen %d (%s) isdef %d\n",
					d->spel, d->spel_asm, gen, decl_to_str(d), d->is_definition);
		}
	}
}

void gen_stmt_code(stmt *s)
{
	stmt **titer;

	/* stmt_for needs to do this too */
	gen_code_decls(s->symtab);

	for(titer = s->codes; titer && *titer; titer++)
		gen_stmt(*titer);
}
