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
#include "out/out.h"
#include "out/lbl.h"
#include "out/asm.h"
#include "gen_style.h"
#include "out/val.h"
#include "out/ctx.h"
#include "cc1_out_ctx.h"

void IGNORE_PRINTGEN(const out_val *v)
{
	(void)v;
}

const out_val *gen_expr(expr *e, out_ctx *octx)
{
	consty k;

	/* always const_fold functions, i.e. builtins */
	if(expr_kind(e, funcall) || fopt_mode & FOPT_CONST_FOLD)
		const_fold(e, &k);
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

const out_val *lea_expr(expr *e, out_ctx *octx)
{
	char buf[WHERE_BUF_SIZ];

	UCC_ASSERT(e->f_lea,
			"invalid store expression expr-%s @ %s (no f_lea())",
			e->f_str(), where_str_r(buf, &e->where));

	out_dbg_where(octx, &e->where);

	return e->f_lea(e, octx);
}

void gen_stmt(stmt *t, out_ctx *octx)
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

			if(fopt_mode & FOPT_VERBOSE_ASM)
				out_comment(octx, "arg %s @ %s", decls[i]->spel, out_val_str(s->outval, 1));
		}
	}
}

static void release_arg_vals(decl **decls, out_ctx *octx)
{
	unsigned i, j;

	for(i = j = 0; decls && decls[i]; i++){
		sym *s = decls[i]->sym;

		if(s && s->type == sym_arg){
			out_val_release(octx, s->outval);
			gen_set_sym_outval(octx, s, NULL);
		}
	}
}

static void allocate_vla_args(out_ctx *octx, symtable *arg_symtab)
{
	decl **i;

	for(i = arg_symtab->decls; i && *i; i++){
		const out_val *dest, *src;
		decl *d = *i;
		type *decayed;
		unsigned vla_space;

		/* generate side-effects even if it's decayed, e.g.
		 * f(int p[E1][E2])
		 * ->
		 * f(int (*p)[E2])
		 *
		 * we still want E1 generated
		 */
		if((decayed = type_is_decayed_array(d->ref))
		&& (decayed = type_is_vla(decayed, VLA_TOP_DIMENSION)))
		{
			out_val_consume(octx, gen_expr(decayed->bits.array.size, octx));
		}

		if(!type_is_variably_modified(d->ref))
			continue;

		/* this array argument is a VLA and needs more size than
		 * just its pointer. we move it to a place on the stack where
		 * we have more space. debug output is unaffected, since we
		 * don't touch the original pointer value, which is all it needs */
		src = out_new_sym_val(octx, d->sym);

		out_comment(octx, "move vla argument %s", d->spel);

		vla_space = vla_decl_space(d);

		out_val_release(octx, d->sym->outval);
		gen_set_sym_outval(octx, d->sym, out_aalloc(
					octx, vla_space, type_align(d->ref, NULL), d->ref));

		dest = out_new_sym(octx, d->sym);
		out_store(octx, dest, src);


		vla_decl_init(d, octx);
	}
}

void gen_set_sym_outval(out_ctx *octx, sym *sym, const out_val *v)
{
	sym->outval = v;

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
		for(aiter = arg_symtab->decls; aiter && *aiter; aiter++){
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

		assign_arg_vals(arg_symtab->decls, argvals, octx);

		allocate_vla_args(octx, arg_symtab);
		free(argvals), argvals = NULL;

		if(cc1_gdebug)
			out_dbg_emit_args_done(octx, type_funcargs(d->ref));

		gen_stmt(d->bits.func.code, octx);

		release_arg_vals(arg_symtab->decls, octx);

		vla_cleanup(octx);

		out_dump_retained(octx, d->spel);

		out_dbg_where(octx, &d->bits.func.code->where_cbrace);

		{
			char *end = out_dbg_func_end(decl_asm_spel(d));
			int stack_used;
			out_func_epilogue(octx, d->ref, end, &stack_used);
			arg_symtab->stack_used = stack_used;
			free(end);
		}

		out_ctx_wipe(octx);

	}else{
		/* asm takes care of .bss vs .data, etc */
		asm_declare_decl_init(d);
	}
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
			asm_declare_stringlit(SECTION_DATA, lit);
}

static int func_pure_inline(decl *d)
{
	if(d->store & store_inline){
		/*
		 * inline semantics
		 *
		 * "" = inline only
		 * "static" = code emitted, decl is static
		 * "extern" mentioned, or "inline" not mentioned = code emitted, decl is extern
		 *
		 * look for non-default store on any prototypes
		 */
		for(; d; d = d->proto)
			if((d->store & STORE_MASK_STORE) != store_default)
				return 0;

		return 1;
	}

	return 0;
}

void gen_asm_global_w_store(decl *d, int emit_tenatives, out_ctx *octx)
{
	int emitted_type = 0;

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
		if(func_pure_inline(d) || !d->bits.func.code){
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

	if(!emitted_type && (d->store & STORE_MASK_STORE) != store_static)
		asm_predeclare_global(d);
	gen_asm_global(d, octx);
}

void gen_asm(
		symtable_global *globs,
		const char *fname, const char *compdir,
		struct out_dbg_filelist **pfilelist)
{
	decl **diter;
	struct symtable_gasm **iasm = globs->gasms;
	out_ctx *octx = out_ctx_new();

	*pfilelist = NULL;

	if(cc1_gdebug)
		out_dbg_begin(octx, &octx->dbg.file_head, fname, compdir);

	for(diter = globs->stab.decls; diter && *diter; diter++){
		decl *d = *diter;

		while(iasm && d == (*iasm)->before){
			gen_gasm((*iasm)->asm_str);

			if(!*++iasm)
				iasm = NULL;
		}

		gen_asm_global_w_store(d, 0, octx);
	}

	for(; iasm && *iasm; ++iasm)
		gen_gasm((*iasm)->asm_str);

	gen_stringlits(globs->literals);

	if(cc1_gdebug){
		out_dbg_end(octx);

		*pfilelist = octx->dbg.file_head;
	}

	cc1_out_ctx_free(octx);
	out_ctx_end(octx);
}
