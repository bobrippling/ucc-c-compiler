#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "../util/platform.h"

#include "macros.h"
#include "../util/util.h"

#include "cc1.h"
#include "sym.h"
#include "const.h"
#include "expr.h"
#include "stmt.h"
#include "vla.h"
#include "type_is.h"
#include "out/dbg.h"
#include "gen_asm.h"
#include "gen_asm_ctors.h"
#include "out/out.h"
#include "out/lbl.h"
#include "out/asm.h"
#include "gen_style.h"
#include "out/val.h"
#include "out/ctx.h"
#include "cc1_out_ctx.h"
#include "inline.h"
#include "type_nav.h"
#include "label.h"
#include "fopt.h"

#include "ops/expr_funcall.h"

int gen_had_error;

void IGNORE_PRINTGEN(const out_val *v)
{
	(void)v;
}

const out_val *gen_expr(const expr *e, out_ctx *octx)
{
	consty k;

	/* always const_fold functions, i.e. builtins */
	if(expr_kind(e, funcall) || cc1_fopt.const_fold)
		const_fold((expr *)e, &k);
	else
		k.type = CONST_NO;

	out_dbg_where(octx, &e->where);

	if(k.type == CONST_NUM){
		/* -O0 skips this? */
		if(cc1_backend == BACKEND_ASM){
			return out_new_num(octx, e->tree_type, &k.bits.num);
		}else{
			stylef("%" NUMERIC_FMT_D, k.bits.num.val.i);
			return NULL;
		}
	}else{
		return e->f_gen(e, octx);
	}
}

void gen_stmt(const stmt *t, out_ctx *octx)
{
	if(octx) /* for other backends */
		out_dbg_where(octx, &t->where);

	t->f_gen(t, octx);
}

static void assign_arg_vals(decl **decls, const out_val *argvals[], out_ctx *octx)
{
	unsigned i, j;

	for(i = j = 0; decls && decls[i]; i++){
		sym *s = decls[i]->sym;

		if(s && s->type == sym_arg){
			gen_set_sym_outval(octx, s, argvals[j++]);

			if(cc1_fopt.verbose_asm){
				out_comment(octx, "arg %s @ %s",
						decls[i]->spel,
						out_val_str(sym_outval(s), 1));
			}
		}
	}
}

static void release_arg_vals(decl **decls, out_ctx *octx)
{
	unsigned i;

	for(i = 0; decls && decls[i]; i++){
		sym *s = decls[i]->sym;

		if(s && s->type == sym_arg){
			out_val_release(octx, sym_outval(s));
			gen_set_sym_outval(octx, s, NULL);
		}
	}
}

void gen_vla_arg_sideeffects(decl *d, out_ctx *octx)
{
	type *decayed;

	if((decayed = type_is_decayed_array(d->ref))
	&& (decayed = type_is_vla(decayed, VLA_TOP_DIMENSION)))
	{
		out_val_consume(octx, gen_expr(decayed->bits.array.size, octx));
	}
}

static void allocate_vla_args(out_ctx *octx, symtable *arg_symtab)
{
	decl **i;

	for(i = symtab_decls(arg_symtab); i && *i; i++){
		const out_val *dest, *src;
		decl *d = *i;
		unsigned vla_space;

		/* generate side-effects even if it's decayed, e.g.
		 * f(int p[E1][E2])
		 * ->
		 * f(int (*p)[E2])
		 *
		 * we still want E1 generated
		 */
		gen_vla_arg_sideeffects(d, octx);

		if(!type_is_variably_modified(d->ref))
			continue;

		/* this array argument is a VLA and needs more size than
		 * just its pointer. we move it to a place on the stack where
		 * we have more space. debug output is unaffected, since we
		 * don't touch the original pointer value, which is all it needs */
		src = out_new_sym_val(octx, d->sym);

		out_comment(octx, "move vla argument %s", d->spel);

		vla_space = vla_decl_space(d);

		out_val_release(octx, sym_outval(d->sym));
		gen_set_sym_outval(octx, d->sym, out_aalloc(
					octx, vla_space, type_align(d->ref, NULL), d->ref));

		dest = out_new_sym(octx, d->sym);
		out_store(octx, dest, src);


		vla_decl_init(d, octx);
	}
}

void gen_set_sym_outval(out_ctx *octx, sym *sym, const out_val *v)
{
	sym_setoutval(sym, v);

	if(v && cc1_gdebug)
		out_dbg_emit_decl(octx, sym->decl, v);
}

static void gen_asm_global(decl *d, out_ctx *octx)
{
	attribute *sec;

	if((sec = attribute_present(d, attr_section))){
		ICW("%s: TODO: section attribute \"%s\" on %s",
				where_str(&sec->where),
				sec->bits.section, d->spel);
	}

	/* order of the if matters */
	if(type_is(d->ref, type_func)){
		int nargs = 0, is_vari;
		decl **aiter;
		const char *sp;
		const out_val **argvals;
		symtable *arg_symtab;

		if(!d->bits.func.code)
			return;

		out_dbg_where(octx, &d->where);

		arg_symtab = DECL_FUNC_ARG_SYMTAB(d);
		for(aiter = symtab_decls(arg_symtab); aiter && *aiter; aiter++){
			decl *d = *aiter;

			if(d->sym->type == sym_arg)
				nargs++;
		}

		argvals = nargs ? umalloc(nargs * sizeof *argvals) : NULL;

		sp = decl_asm_spel(d);

		out_func_prologue(octx, sp, d->ref,
				nargs,
				is_vari = type_is_variadic_func(d->ref),
				argvals);

		assign_arg_vals(symtab_decls(arg_symtab), argvals, octx);

		allocate_vla_args(octx, arg_symtab);
		free(argvals), argvals = NULL;

		if(cc1_gdebug)
			out_dbg_emit_args_done(octx, type_funcargs(d->ref));

		gen_func_stmt(d->bits.func.code, octx);

		release_arg_vals(symtab_decls(arg_symtab), octx);

		/* vla cleanup for an entire function - inlined
		 * VLAs need to retain their uniqueness across inline calls */
		vla_cleanup(octx);

		label_cleanup(octx);

		out_dbg_where(octx, &d->bits.func.code->where_cbrace);

		{
			char *end = out_dbg_func_end(decl_asm_spel(d));
			int stack_used;
			out_func_epilogue(octx, d->ref, end, &stack_used);
			arg_symtab->stack_used = stack_used;
			free(end);
		}

		if(out_dump_retained(octx, d->spel))
			gen_had_error = 1;

		out_ctx_wipe(octx);

	}else{
		/* asm takes care of .bss vs .data, etc */
		asm_declare_decl_init(d);
	}
}

const out_val *gen_call(
		expr *maybe_exp, decl *maybe_dfn,
		const out_val *fnval,
		const out_val **args, out_ctx *octx,
		const where *loc)
{
	const char *whynot;
	const out_val *fn_ret;

	/* (re-)emit line location - function calls are commonly split
	 * over multiple lines, so we want the debugger to stop again
	 * on the top line when we're about to emit the call */
	out_dbg_where(octx, loc);

	fn_ret = inline_func_try_gen(
			maybe_exp, maybe_dfn, fnval, args, octx, &whynot, loc);

	if(fn_ret){
		if(cc1_fopt.show_inlined)
			note_at(loc, "function inlined");

	}else{
		const int always_inline = !!(maybe_exp
			? expr_attr_present(maybe_exp, attr_always_inline)
			: attribute_present(maybe_dfn, attr_always_inline));

		if(always_inline){
			warn_at_print_error(loc, "couldn't always_inline call: %s", whynot);

			gen_had_error = 1;
		}

		fn_ret = out_call(octx, fnval, args,
				maybe_exp ? maybe_exp->tree_type : type_ptr_to(maybe_dfn->ref));
	}

	return fn_ret;
}

const out_val *gen_decl_addr(out_ctx *octx, decl *d)
{
	int local = decl_linkage(d) == linkage_internal;

	if(!local){
		if(type_is(d->ref, type_func)){
			/* defined functions, even though exported, are local. unless weak */
			local = d->bits.func.code && !attribute_present(d, attr_weak);
		}else{
			/* variable - if initialised then it's not interposable */
			local = d->bits.var.init.dinit && !d->bits.var.init.compiler_generated;
		}
	}

	return out_new_lbl(
			octx,
			type_ptr_to(d->ref),
			decl_asm_spel(d),
			OUT_LBL_PIC | (local ? OUT_LBL_PICLOCAL : 0));
}

static void gen_gasm(char *asm_str)
{
	fprintf(cc_out[SECTION_TEXT], "%s\n", asm_str);
}

static void gen_stringlits(dynmap *litmap)
{
	const stringlit *lit;
	size_t i;
	for(i = 0; (lit = dynmap_value(stringlit *, litmap, i)); i++)
		if(lit->use_cnt > 0)
			asm_declare_stringlit(SECTION_RODATA, lit);
}

void gen_asm_emit_type(out_ctx *octx, type *ty)
{
	/* for types that aren't on variables (e.g. in exprs),
	 * that debug info may not find out about normally */
	if(cc1_gdebug && type_is_s_or_u(ty))
		out_dbg_emit_type(octx, ty);
}

void gen_asm_global_w_store(decl *d, int emit_tenatives, out_ctx *octx)
{
	struct cc1_out_ctx *cc1_octx = *cc1_out_ctx(octx);
	int emitted_type = 0;

	/* in map? */
	if(cc1_octx && dynmap_exists(decl *, cc1_octx->generated_decls, d))
		return;

	/* add to map */
	cc1_octx = cc1_out_ctx_or_new(octx);
	if(!cc1_octx->generated_decls)
		cc1_octx->generated_decls = dynmap_new(decl *, /*ref*/NULL, decl_hash);
	(void)dynmap_set(decl *, int *, cc1_octx->generated_decls, d, (int *)NULL);

	if(decl_asm_spel(d)){
		if(!cc1_octx->spel_to_fndecl){
			cc1_octx->spel_to_fndecl = dynmap_new(
					const char *, strcmp, dynmap_strhash);
		}

		(void)dynmap_set(
				const char *, decl *,
				cc1_octx->spel_to_fndecl,
				decl_asm_spel(d), d);
	}

	switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
		case store_inline:
		case store_auto:
		case store_register:
			ICE("%s storage on global %s",
					decl_store_to_str(d->store),
					decl_to_str(d));

		case store_typedef:
			return;

		case store_extern:
		case store_default:
		case store_static:
			break;
	}

	if(attribute_present(d, attr_weak)){
		asm_predeclare_weak(d);
		emitted_type = 1;
	}

	if(type_is(d->ref, type_func)){
		if(!decl_should_emit_code(d)){
			/* inline only gets extern emitted anyway */
			if(!emitted_type)
				asm_predeclare_extern(d);
			return;
		}

		if(cc1_gdebug)
			out_dbg_emit_func(octx, d);
	}else{
		/* variable - if there's no init,
		 * it's tenative and not output
		 *
		 * unless we're told to emit tenatives, e.g. local scope
		 */
		if(!emit_tenatives && !d->bits.var.init.dinit){
			if(!emitted_type)
				asm_predeclare_extern(d);
			return;
		}

		if(cc1_gdebug)
			out_dbg_emit_global_var(octx, d);
	}

	if(!emitted_type && decl_linkage(d) == linkage_external)
		asm_predeclare_global(d);
	gen_asm_global(d, octx);
}

void gen_asm(
		symtable_global *globs,
		const char *fname, const char *compdir,
		struct out_dbg_filelist **pfilelist)
{
	decl **inits = NULL, **terms = NULL;
	decl **diter;
	struct symtable_gasm **iasm = globs->gasms;
	out_ctx *octx = out_ctx_new();

	*pfilelist = NULL;

	if(cc1_gdebug)
		out_dbg_begin(octx, &octx->dbg.file_head, fname, compdir, cc1_std);

	for(diter = symtab_decls(&globs->stab); diter && *diter; diter++){
		decl *d = *diter;

		while(iasm && d == (*iasm)->before){
			gen_gasm((*iasm)->asm_str);

			if(!*++iasm)
				iasm = NULL;
		}

		gen_asm_global_w_store(d, 0, octx);

		if(type_is(d->ref, type_func) && d->bits.func.code){
			if(attribute_present(d, attr_constructor))
				dynarray_add(&inits, d);
			if(attribute_present(d, attr_destructor))
				dynarray_add(&terms, d);
		}
	}

	for(; iasm && *iasm; ++iasm)
		gen_gasm((*iasm)->asm_str);

	gen_stringlits(globs->literals);

	gen_inits_terms(inits, terms);
	dynarray_free(decl **, inits, NULL);
	dynarray_free(decl **, terms, NULL);

	if(cc1_gdebug){
		out_dbg_end(octx);

		*pfilelist = octx->dbg.file_head;
	}

	cc1_out_ctx_free(octx);
	out_ctx_end(octx);
}
