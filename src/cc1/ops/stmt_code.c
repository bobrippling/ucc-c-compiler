#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_code.h"
#include "../out/lbl.h"
#include "../decl_init.h"
#include "../../util/dynarray.h"

const char *str_stmt_code()
{
	return "code";
}

void fold_stmt_code(stmt *s)
{
	decl **diter;
	stmt **siter;
	stmt *inits = NULL;
	int warned = 0;

	fold_symtab_scope(s->symtab);

	/* NOTE: this only folds + adds decls, not args */
	for(diter = s->decls; diter && *diter; diter++){
		decl *d = *diter;

		if(d->func_code)
			DIE_AT(&d->func_code->where, "can't nest functions");
		else if(DECL_IS_FUNC(d) && (d->store & STORE_MASK_STORE) == store_static)
			DIE_AT(&d->where, "block-scoped function cannot have static storage");

		fold_decl(d, s->symtab);

		d->is_definition = 1; /* always the def for non-globals */

		/* must be before fold*, since sym lookups are done */
		SYMTAB_ADD(s->symtab, d,
				decl_store_static_or_extern(d->store) ? sym_global : sym_local);

		if(d->init){
			/* this creates the below s->inits array */
			if((d->store & STORE_MASK_STORE) == store_static){
				fold_decl_global_init(d, s->symtab);
			}else{
				EOF_WHERE(&d->where,
						if(!inits)
							inits = stmt_new_wrapper(code, symtab_new(s->symtab));

						decl_init_brace_up_fold(d, inits->symtab);
						decl_init_create_assignments_base(d->init,
							d->ref, expr_new_identifier(d->spel),
							inits);
					);
				/* folded below */
			}
		}
	}

	if(inits)
		dynarray_prepend(&s->codes, inits);

	for(siter = s->codes; siter && *siter; siter++){
		stmt *const st = *siter;

		EOF_WHERE(&st->where, fold_stmt(st));

		/*
		 * check for dead code
		 */
		if(!warned
		&& st->kills_below_code
		&& siter[1]
		&& !stmt_kind(siter[1], label)
		&& !stmt_kind(siter[1], case)
		&& !stmt_kind(siter[1], default)
		){
			cc1_warn_at(&siter[1]->where, 0, 1, WARN_DEAD_CODE, "dead code after %s (%s)", st->f_str(), siter[1]->f_str());
			warned = 1;
		}
	}

	/* static folding */
	if(s->decls){
		decl **siter;

		for(siter = s->decls; *siter; siter++){
			decl *d = *siter;
			/*
			 * check static decls - after we fold,
			 * so we've linked the syms and can change ->spel
			 */
			if((d->store & STORE_MASK_STORE) == store_static){
				char *old = d->spel;
				d->spel = out_label_static_local(curdecl_func->spel, d->spel);
				free(old);
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

		if((func = !!type_ref_is(d->ref, type_ref_func)) || decl_store_static_or_extern(d->store)){
			int gen = 1;

			if(func){
				/* check if the func is defined globally */
				symtable *globs = symtab_root(stab);
				decl **i;

				for(i = globs->decls; i && *i; i++){
					if(!strcmp(d->spel, (*i)->spel)){
						gen = 0;
						break;
					}
				}
			}

			if(gen)
				gen_asm_global(d);
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

static int code_passable(stmt *s)
{
	stmt **i;

	/* note: this also checks for inits which call noreturn funcs */

	for(i = s->codes; i && *i; i++){
		stmt *sub = *i;
		if(!fold_passable(sub))
			return 0;
	}

	return 1;
}

void mutate_stmt_code(stmt *s)
{
	s->f_passable = code_passable;
}
