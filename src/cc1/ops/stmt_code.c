#include <stdlib.h>
#include <string.h>

#include "ops.h"
#include "stmt_code.h"
#include "../decl_init.h"
#include "../../util/dynarray.h"
#include "../fold_sym.h"
#include "../out/lbl.h"
#include "../type_is.h"

const char *str_stmt_code()
{
	return "code";
}

void fold_block_decls(symtable *stab, stmt **pinit_blk)
{
	/* must iterate using an index, since the array may
	 * resize under us */
	size_t i;

	if(!stab->decls)
		return;

	/* check for invalid function redefinitions and shadows */
	for(i = 0; stab->decls[i]; i++){
		decl *const d = stab->decls[i];
		decl *found;
		symtable *above_scope;
		int chk_shadow = 0, is_func = 0;

		fold_decl(d, stab, pinit_blk);

		if((is_func = !!type_is(d->ref, type_func)))
			chk_shadow = 1;
		else if(warn_mode & (WARN_SHADOW_LOCAL | WARN_SHADOW_GLOBAL))
			chk_shadow = 1;

		if(chk_shadow
		&& d->spel
		&& (found = symtab_search_d(stab->parent, d->spel, &above_scope)))
		{
			char buf[WHERE_BUF_SIZ];

			/* allow functions redefined as decls and vice versa */
			if(is_func
			&& type_is(found->ref, type_func)
			&& !(decl_cmp(d, found, 0) & TYPE_EQUAL_ANY))
			{
				die_at(&d->where,
						"incompatible redefinition of \"%s\"\n"
						"%s: note: previous definition",
						d->spel, where_str_r(buf, &found->where));
			}else{
				const int same_scope = symtab_nested_internal(above_scope, stab);

				/* same scope? error unless they're both extern */
				if(same_scope && (
					d->store != found->store
					|| (STORE_MASK_STORE & d->store) != store_extern))
				{
					die_at(&d->where, "redefinition of \"%s\"\n"
							"%s: note: previous definition here",
							d->spel, where_str_r(buf, &found->where));
				}

				/* -Wshadow:
				 * if it has a parent, we found it in local scope, so check the local mask
				 * and vice versa
				 */
				if(warn_mode & (
					above_scope->parent ? WARN_SHADOW_LOCAL : WARN_SHADOW_GLOBAL))
				{
					const char *ty = above_scope->parent ? "local" : "global";

					warn_at(&d->where,
							"declaration of \"%s\" shadows %s declaration\n"
							"%s: note: %s declaration here",
							d->spel, ty,
							where_str_r(buf, &found->where), ty);
				}
			}
		}
	}
}

void stmt_code_got_decls(stmt *code)
{
	stmt *init_blk = NULL;

	fold_block_decls(code->symtab, &init_blk);

	if(init_blk)
		dynarray_prepend(&code->bits.code.stmts, init_blk);
}

void fold_stmt_code(stmt *s)
{
	stmt **siter;
	int warned = 0;

	/* local struct layout-ing */
	/* we fold decls ourselves, to get their inits */
	symtab_fold_sues(s->symtab);

	for(siter = s->bits.code.stmts; siter && *siter; siter++){
		stmt *const st = *siter;

		fold_stmt(st);

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

void gen_block_decls(symtable *stab, const char **dbg_end_lbl)
{
	decl **diter;

	if(cc1_gdebug && !stab->lbl_begin){
		stab->lbl_begin = out_label_code("dbg_begin");
		stab->lbl_end = out_label_code("dbg_end");

		out_label_noop(stab->lbl_begin);
		*dbg_end_lbl = stab->lbl_end;
	}else{
		*dbg_end_lbl = NULL;
	}

	/* declare strings, extern functions and blocks */
	for(diter = stab->decls; diter && *diter; diter++){
		decl *d = *diter;
		int func;

		/* we may need a '.extern fn...' for prototypes... */
		if((func = !!type_is(d->ref, type_func))
		|| decl_store_static_or_extern(d->store))
		{
			/* if it's a string, go,
			 * if it's the most-unnested func. prototype, go */
			if(!func || !d->proto)
				gen_asm_global(d);
		}
	}
}

void gen_stmt_code(stmt *s)
{
	stmt **titer;
	const char *endlbl;

	/* stmt_for/if/while/do needs to do this too */
	gen_block_decls(s->symtab, &endlbl);

	for(titer = s->bits.code.stmts; titer && *titer; titer++)
		gen_stmt(*titer);

	if(endlbl)
		out_label_noop(endlbl);
}

void style_stmt_code(stmt *s)
{
	stmt **i_s;
	decl **i_d;

	stylef("{\n");

	for(i_d = s->symtab->decls; i_d && *i_d; i_d++)
		gen_style_decl(*i_d);

	for(i_s = s->bits.code.stmts; i_s && *i_s; i_s++)
		gen_stmt(*i_s);

	stylef("\n}\n");
}

static int code_passable(stmt *s)
{
	stmt **i;

	/* note: this also checks for inits which call noreturn funcs */

	for(i = s->bits.code.stmts; i && *i; i++){
		stmt *sub = *i;
		if(!fold_passable(sub))
			return 0;
	}

	return 1;
}

void init_stmt_code(stmt *s)
{
	s->f_passable = code_passable;
}
