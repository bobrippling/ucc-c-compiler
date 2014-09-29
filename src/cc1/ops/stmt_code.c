#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "ops.h"
#include "stmt_code.h"
#include "../decl_init.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../fold_sym.h"
#include "../out/lbl.h"
#include "../type_is.h"
#include "../type_nav.h"
#include "../out/dbg.h"
#include "../vla.h"

/* out_alloca_fixed() */
#include "../out/out.h"
#include "../cc1_out_ctx.h"

const char *str_stmt_code()
{
	return "code";
}

static void cleanup_check(decl *d, attribute *cleanup)
{
	decl *fn = cleanup->bits.cleanup;
	funcargs *args;
	int nargs;
	type *targ, *expected;

	/* fn can return anything, but must take a single argument */
	if(!type_is(fn->ref, type_func)){
		fold_had_error = 1;
		warn_at_print_error(&cleanup->where, "cleanup is not a function");
		return;
	}

	type_called(fn->ref, &args);
	if((nargs = dynarray_count(args->arglist)) != 1){
		fold_had_error = 1;
		warn_at_print_error(&cleanup->where,
				"cleanup needs one argument (not %d)", nargs);
		return;
	}

	expected = args->arglist[0]->ref;
	targ = type_ptr_to(d->ref);
	if(!(type_cmp(targ, expected, 0) & TYPE_EQUAL_ANY)
	&& !type_is_void_ptr(expected))
	{
		char targ_buf[TYPE_STATIC_BUFSIZ];
		char expected_buf[TYPE_STATIC_BUFSIZ];

		fold_had_error = 1;
		warn_at_print_error(&cleanup->where,
				"type '%s' passed - cleanup needs '%s'",
				type_to_str_r(targ_buf, targ),
				type_to_str_r(expected_buf, expected));
		return;
	}
}

void fold_shadow_dup_check_block_decls(symtable *stab)
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
		attribute *attr;

		fold_decl(d, stab);

		/* block decls must be complete */
		fold_check_decl_complete(d);

		if((attr = attribute_present(d, attr_cleanup)))
			cleanup_check(d, attr);

		if((is_func = !!type_is(d->ref, type_func)))
			chk_shadow = 1;
		else if(cc1_warning.shadow_local || cc1_warning.shadow_global)
			chk_shadow = 1;

		if(chk_shadow
		&& d->spel
		&& (found = symtab_search_d(stab->parent, d->spel, &above_scope)))
		{
			char buf[WHERE_BUF_SIZ];
			int both_func = is_func && type_is(found->ref, type_func);
			int both_extern = decl_linkage(d) == linkage_external
				&& decl_linkage(found) == linkage_external;

			/* allow functions redefined as decls and vice versa */
			if((both_func || both_extern)
			&& !(decl_cmp(d, found, 0) & TYPE_EQUAL_ANY))
			{
				die_at(&d->where,
						"incompatible redefinition of \"%s\"\n"
						"%s: note: previous definition",
						d->spel, where_str_r(buf, &found->where));
			}else{
				const int same_scope = symtab_nested_internal(above_scope, stab);

				/* same scope? error unless they're both extern */
				if(same_scope && !both_extern){
					die_at(&d->where, "redefinition of \"%s\"\n"
							"%s: note: previous definition here",
							d->spel, where_str_r(buf, &found->where));
				}

				/* -Wshadow:
				 * if it has a parent, we found it in local scope, so check the local mask
				 * and vice versa
				 */
				if(above_scope->parent
						? cc1_warning.shadow_local
						: cc1_warning.shadow_global)
				{
					const char *ty = above_scope->parent ? "local" : "global";

					/* unconditional warning - checked above */
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

void fold_stmt_code(stmt *s)
{
	stmt **siter;
	int warned = 0;
	enum decl_storage func_store;

	/* local struct layout-ing */
	symtab_fold_sues(s->symtab);

	func_store = symtab_func(s->symtab)->store;
	if(func_store & store_inline
	&& (func_store & STORE_MASK_STORE) == store_default)
	{
		decl **diter;
		for(diter = s->symtab->decls; diter && *diter; diter++){
			decl *d = *diter;

			if((d->store & STORE_MASK_STORE) == store_static
			&& !type_is_const(d->ref))
			{
				cc1_warn_at(&d->where, static_local_in_inline,
						"mutable static variable in pure-inline function - may differ per file");
			}
		}
	}

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
			cc1_warn_at(&siter[1]->where, dead_code,
					"dead code after %s (%s)", st->f_str(), siter[1]->f_str());
			warned = 1;
		}
	}
}

static void gen_auto_decl_alloc(decl *d, out_ctx *octx)
{
	sym *s = d->sym;
	const enum decl_storage store = (d->store & STORE_MASK_STORE);

	if(!s || s->type != sym_local)
		return;

	assert(!type_is(d->ref, type_func));

	switch(store){
		case store_register:
		case store_default:
		case store_auto:
		case store_typedef:
		{
			unsigned siz;
			unsigned align;
			const int vm = type_is_variably_modified(s->decl->ref);

			if(vm){
				siz = vla_decl_space(s->decl);
				align = platform_word_size();
			}else if(store == store_typedef){
				/* non-VM typedef - no space needed */
				break;
			}else{
				siz = decl_size(s->decl);
				align = decl_align(s->decl);
			}

			assert(!s->outval);
			gen_set_sym_outval(s, out_aalloc(octx, siz, align, s->decl->ref));
			break;
		}

		case store_static:
		case store_extern:
		case store_inline:
			break;
	}
}

static void gen_auto_decl(decl *d, out_ctx *octx)
{
	gen_auto_decl_alloc(d, octx);

	if(type_is_variably_modified(d->ref)){
		if((d->store & STORE_MASK_STORE) == store_typedef)
			vla_typedef_init(d, octx);
		else
			vla_decl_init(d, octx);
	}
}

void gen_block_decls(
		symtable *stab, const char **dbg_end_lbl, out_ctx *octx)
{
	decl **diter;

	if(cc1_gdebug && !stab->lbl_begin){
		stab->lbl_begin = out_label_code("dbg_begin");
		stab->lbl_end = out_label_code("dbg_end");

		out_dbg_label(octx, stab->lbl_begin);
		*dbg_end_lbl = stab->lbl_end;
	}else{
		*dbg_end_lbl = NULL;
	}

	/* declare strings, extern functions, blocks and vlas */
	for(diter = stab->decls; diter && *diter; diter++){
		decl *d = *diter;
		int func;

		/* we may need a '.extern fn...' for prototypes... */
		if((func = !!type_is(d->ref, type_func))
		|| decl_store_duration_is_static(d))
		{
			/* if it's a string, go,
			 * if it's the most-unnested func. prototype, go */
			if(!func || !d->proto)
				gen_asm_global_w_store(d, 1, octx);
			continue;
		}

		gen_auto_decl(d, octx);

		/* check .expr, since empty structs .expr == NULL */
		if(d->bits.var.init.expr
		&& /*anonymous decls are initialised elsewhere:*/d->spel)
		/*(anonymous decls are like those from compound literals)*/
		{
			out_val_consume(octx,
					gen_expr(d->bits.var.init.expr, octx));
		}
	}
}

void gen_block_decls_dealloca(symtable *stab, out_ctx *octx)
{
	decl **diter;

	for(diter = stab->decls; diter && *diter; diter++){
		decl *d = *diter;
		int is_typedef;

		if(!d->sym || d->sym->type != sym_local || type_is(d->ref, type_func))
			continue;

		is_typedef = ((d->store & STORE_MASK_STORE) == store_typedef);

		/* typedefs may or may not have a sym */
		if(is_typedef && !d->sym->outval)
			continue;

		if(!is_typedef && type_is_vla(d->ref, VLA_ANY_DIMENSION))
			out_alloca_pop(octx);

		out_adealloc(octx, &d->sym->outval);
	}
}

static void gen_scope_destructors(symtable *scope, out_ctx *octx)
{
	decl **di;

	if(!scope->decls)
		return;

	for(di = scope->decls; *di; di++);
	do{
		decl *d;

		di--;
		d = *di;

		if(d->sym){
			attribute *cleanup = attribute_present(d, attr_cleanup);

			if(cleanup){
				const out_val *args[2];
				type *fty = cleanup->bits.cleanup->ref;

				out_dbg_where(octx, &d->where);

				args[0] = out_new_sym(octx, d->sym);
				args[1] = NULL;

				out_flush_volatile(octx,
						out_call(
							octx,
							out_new_lbl(octx, NULL, decl_asm_spel(cleanup->bits.cleanup), 1),
							args,
							type_ptr_to(fty)));
			}

			if(((d->store & STORE_MASK_STORE) != store_typedef)
			&& type_is_vla(d->ref, VLA_ANY_DIMENSION))
			{
				out_alloca_restore(octx, vla_saved_ptr(d, octx));
			}
		}
	}while(di != scope->decls);
}

#define SYMTAB_PARENT_WALK(it, begin) \
	for(it = begin; it; it = it->parent)

static void mark_symtabs(symtable *const s_from, int m)
{
	symtable *s_iter;
	SYMTAB_PARENT_WALK(s_iter, s_from)
		s_iter->mark = m;
}

void fold_check_scope_entry(where *w, const char *desc,
		symtable *const s_from, symtable *const s_to)
{
	symtable *s_iter;

	mark_symtabs(s_from, 1);

	SYMTAB_PARENT_WALK(s_iter, s_to){
		decl **i;

		if(s_iter->mark)
			break;

		for(i = s_iter->decls; i && *i; i++){
			decl *d = *i;
			if(type_is_variably_modified(d->ref)){
				char buf[WHERE_BUF_SIZ];

				fold_had_error = 1;
				warn_at_print_error(w,
						"%s scope of variably modified declaration\n"
						"%s: note: variable \"%s\"",
						desc, where_str_r(buf, &d->where), d->spel);
			}
		}
	}

	mark_symtabs(s_from, 0);
}

void gen_scope_leave(
		symtable *const s_from, symtable *const s_to,
		out_ctx *octx)
{
	symtable *s_iter;

	if(!s_to){ /* e.g. return */
		gen_scope_destructors(s_from, octx);
		return;
	}

	mark_symtabs(s_to, 1);

	SYMTAB_PARENT_WALK(s_iter, s_from){
		/* walk up until we hit a mark - that's our target scope
		 * generate destructors along the way
		 */
		if(s_iter->mark)
			break;

		gen_scope_destructors(s_iter, octx);
	}

	mark_symtabs(s_to, 0);
}

void gen_scope_leave_parent(symtable *s_from, out_ctx *octx)
{
	gen_scope_leave(s_from, s_from->parent, octx);
}

void gen_stmt_code_m1_finish(const stmt *s, out_ctx *octx)
{
	gen_scope_leave_parent(s->symtab, octx);
	gen_block_decls_dealloca(s->symtab, octx);
}

/* this is done for lea_expr_stmt(), i.e.
 * struct A x = ({ struct A y; y.i = 1; y; });
 * so we can lea the final expr
 */
void gen_stmt_code_m1(const stmt *s, int m1, out_ctx *octx)
{
	stmt **titer;
	const char *endlbl;

	/* stmt_for/if/while/do needs to do this too */
	gen_block_decls(s->symtab, &endlbl, octx);

	for(titer = s->bits.code.stmts; titer && *titer; titer++){
		if(m1 && !titer[1])
			break;
		gen_stmt(*titer, octx);
	}

	if(!m1)
		gen_stmt_code_m1_finish(s, octx);
	/* else the caller should do ^ */

	if(endlbl)
		out_dbg_label(octx, endlbl);
}

void gen_stmt_code(const stmt *s, out_ctx *octx)
{
	gen_stmt_code_m1(s, 0, octx);
}

void style_stmt_code(const stmt *s, out_ctx *octx)
{
	stmt **i_s;
	decl **i_d;

	stylef("{\n");

	for(i_d = s->symtab->decls; i_d && *i_d; i_d++)
		gen_style_decl(*i_d);

	for(i_s = s->bits.code.stmts; i_s && *i_s; i_s++)
		gen_stmt(*i_s, octx);

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
