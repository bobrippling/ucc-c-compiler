#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_code.h"
#include "../decl_init.h"
#include "../../util/dynarray.h"
#include "../fold_sym.h"

const char *str_stmt_code()
{
	return "code";
}

void fold_stmt_code(stmt *s)
{
	stmt **siter;
	decl **diter;
	stmt *init_blk = NULL;
	int warned = 0;

	/* local struct layout-ing */
	/* we fold decls ourselves, to get their inits */
	symtab_fold_sues(s->symtab);

	/* check for invalid function redefinitions */
	for(diter = s->symtab->decls; diter && *diter; diter++){
		decl *const d = *diter;
		decl *found;

		fold_decl(d, s->symtab, &init_blk);

		if(DECL_IS_FUNC(d)
		&& (found = symtab_search_d(s->symtab->parent, d->spel)))
		{
			/* allow functions redefined as decls and vice versa */
			if(DECL_IS_FUNC(found) && decl_cmp(d, found, 0) != TYPE_EQUAL){
				char buf[WHERE_BUF_SIZ];

				die_at(&d->where,
						"incompatible redefinition of \"%s\"\n"
						"%s: note: previous definition",
						d->spel, where_str_r(buf, &found->where));
			}
		}
	}

	if(init_blk)
		dynarray_prepend(&s->codes, init_blk);

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
			cc1_warn_at(&siter[1]->where, 0, WARN_DEAD_CODE,
					"dead code after %s (%s)", st->f_str(), siter[1]->f_str());
			warned = 1;
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

basic_blk *gen_stmt_code(stmt *s, basic_blk *bb)
{
	stmt **titer;

	/* stmt_for/if/while/do needs to do this too */
	gen_code_decls(s->symtab);

	for(titer = s->codes; titer && *titer; titer++)
		bb = gen_stmt(*titer, bb);

	return bb;
}

basic_blk *style_stmt_code(stmt *s, basic_blk *bb)
{
	stmt **i_s;
	decl **i_d;

	stylef("{\n");

	for(i_d = s->symtab->decls; i_d && *i_d; i_d++)
		gen_style_decl(*i_d);

	for(i_s = s->codes; i_s && *i_s; i_s++)
		bb = gen_stmt(*i_s, bb);

	stylef("\n}\n");

	return bb;
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
