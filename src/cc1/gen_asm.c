#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "../util/macros.h"
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
#include "out/write.h"
#include "cc1_out_ctx.h"
#include "inline.h"
#include "type_nav.h"
#include "label.h"
#include "fopt.h"
#include "cc1_out.h"
#include "cc1_target.h"
#include "sanitize.h"

#include "ops/expr_funcall.h"

int gen_had_error;

#include "type_nav.h"
#include "funcargs.h"

void IGNORE_PRINTGEN(const out_val *v)
{
	(void)v;
}

const out_val *gen_expr(const expr *e, out_ctx *octx)
{
	consty k;
	const out_val *generated;

	/* always const_fold functions, i.e. builtins */
	if(expr_kind(e, funcall) || cc1_fopt.const_fold)
		const_fold((expr *)e, &k);
	else
		k.type = CONST_NO;

	out_dbg_where(octx, &e->where);

	if(k.type == CONST_NUM){
		/* -O0 skips this? */
		if(cc1_backend == BACKEND_ASM){
			generated = out_new_num(octx, e->tree_type, &k.bits.num);
		}else{
			stylef("%" NUMERIC_FMT_D, k.bits.num.val.i);
			return NULL;
		}
	}else{
		generated = e->f_gen(e, octx);
	}

	if(UCC_DEBUG_BUILD && 0/* this is too brittle and coupled to lval decay, etc */){
		type *expected = e->tree_type;
		if(expr_is_lval(e) != LVALUE_NO)
			expected = type_decay(expected);

		if(type_cmp(generated->t, expected, 0) & TYPE_NOT_EQUAL){
			char buf[TYPE_STATIC_BUFSIZ];
			ICE("%s: expected %s to generate '%s' value, got '%s'",
					where_str(&e->where),
					e->f_str(),
					type_to_str(e->tree_type),
					type_to_str_r(buf, generated->t));
		}
	}

	return generated;
}

void gen_stmt(const stmt *t, out_ctx *octx)
{
	if(octx) /* for other backends */
		out_dbg_where(octx, &t->where);

	t->f_gen(t, octx);

	if(octx){
		/* this aids in debugging loops with no body or no test/increment, where
		 * the debugger would otherwise only see a single line, and so continue
		 * until the loop completed.
		 *
		 * for(;;){
		 *   v++
		 * } // we emit this location before the jump to the body
		 *
		 * for(i = 0; i < n; i++) {
		 *   ;
		 * } // we emit this location before the jump to the test
		 */
		out_dbg_where(octx, &t->where_cbrace);
		out_dbg_flush(octx);
	}
}

static void assign_arg_vals(decl **decls, const out_val *argvals[], out_ctx *octx)
{
	unsigned i, j;

	for(i = j = 0; decls && decls[i]; i++){
		sym *s = decls[i]->sym;

		if(s && s->type == sym_arg){
			gen_set_sym_outval(octx, s, argvals[j++]);

			if(cc1_fopt.dump_frame_layout){
				const char *loc = out_val_str(sym_outval(s), 1);

				fprintf(stderr, "frame: %9s: %s (argument)\n",
						loc,
						decls[i]->spel);
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
		out_comment(octx, "vla arg side-effect");
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
		long offset;

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
		gen_set_sym_outval(
			octx,
			d->sym,
			out_aalloc(
				octx,
				vla_space,
				type_align(d->ref, NULL),
				d->ref,
				&offset
			)
		);

		dest = out_new_sym(octx, d->sym);
		out_store(octx, dest, src);


		vla_decl_init(d, octx);
	}
}

void gen_set_sym_outval(out_ctx *octx, sym *sym, const out_val *v)
{
	sym_setoutval(sym, v);

	if(v && cc1_gdebug == DEBUG_FULL)
		out_dbg_emit_decl(octx, sym->decl, v);
}

static int should_stack_protect(decl *d)
{
	unsigned bytes;
	int addr_taken = 0;

	assert(type_is(d->ref, type_func));

	if(attribute_present(d, attr_no_stack_protector))
		return 0;

	if(cc1_fopt.stack_protector_all)
		return 1;

	if(!cc1_fopt.stack_protector)
		return 0;

	if(attribute_present(d, attr_stack_protect))
		return 1;

	/* calls alloca() [TODO] or has an array, or local variable whose address is taken */
	bytes = symtab_decl_bytes(d->bits.func.code->symtab, 8, 1, &addr_taken);
	return bytes >= 8 || addr_taken;
}

static void gen_profile(out_ctx *octx, const char *fn)
{
	type *fnty = type_ptr_to(
			type_func_of(
				type_nav_btype(cc1_type_nav, type_void),
				funcargs_new_void(),
				NULL));

	out_val *fnv = out_new_lbl(
			octx,
			fnty,
			fn, /* not subject to mangling */
			OUT_LBL_PIC);

	out_val_consume(octx, out_call(octx, fnv, NULL, fnty));
}

static void gen_profile_mcount(out_ctx *octx)
{
	if(!cc1_profileg || mopt_mode & MOPT_FENTRY)
		return;
	gen_profile(octx, "mcount");
}

static void gen_profile_fentry(out_ctx *octx)
{
	if(!cc1_profileg || !(mopt_mode & MOPT_FENTRY))
		return;

	out_current_blk(octx, out_blk_entry(octx));
	gen_profile(octx, "__fentry__");
	out_current_blk(octx, out_blk_postprologue(octx));
}

static void gen_type_and_size(const struct section *section, decl *d)
{
	const int is_code = !!type_is(d->ref, type_func);
	const char *spel = decl_asm_spel(d);

	asm_out_section(section, ".type %s,@%s\n",
			spel,
			is_code ? "function" : "object");

	if(is_code)
		asm_out_section(section, ".size %s, .-%s\n", spel, spel);
	else
		asm_out_section(section, ".size %s, %u\n", spel, decl_size(d));
}

static void gen_asm_global(const struct section *section, decl *d, out_ctx *octx)
{
	if(type_is(d->ref, type_func)){
		int nargs = 0, is_vari;
		decl **aiter;
		const char *sp;
		const out_val **argvals;
		symtable *arg_symtab;
		int bail = 0;

		if(!d->bits.func.code)
			return;

		out_dbg_where(octx, &d->where);

		arg_symtab = DECL_FUNC_ARG_SYMTAB(d);
		for(aiter = symtab_decls(arg_symtab); aiter && *aiter; aiter++){
			decl *arg = *aiter;
			struct_union_enum_st *su;

			if(arg->sym->type == sym_arg)
				nargs++;

			if((su = type_is_s_or_u(arg->ref))){
				warn_at_print_error(
						&arg->where,
						"%s arguments are not yet implemented",
						sue_str_type(su->primitive));
				gen_had_error = 1;
				bail = 1;
			}
		}
		if(bail)
			return;

		argvals = nargs ? umalloc(nargs * sizeof *argvals) : NULL;

		sp = decl_asm_spel(d);

		is_vari = type_is_variadic_func(d->ref);

		out_perfunc_init(octx, d, sp);

		gen_profile_fentry(octx);

		out_func_prologue(octx,
				nargs, is_vari,
				should_stack_protect(d),
				argvals);

		assign_arg_vals(symtab_decls(arg_symtab), argvals, octx);

		gen_profile_mcount(octx);

		allocate_vla_args(octx, arg_symtab);
		free(argvals), argvals = NULL;

		if(cc1_gdebug == DEBUG_FULL)
			out_dbg_emit_args_done(octx, type_funcargs(d->ref));

		sanitize_nonnull_args(arg_symtab, octx);

		gen_func_stmt(d->bits.func.code, octx);

		release_arg_vals(symtab_decls(arg_symtab), octx);

		/* vla cleanup for an entire function - inlined
		 * VLAs need to retain their uniqueness across inline calls */
		vla_cleanup(octx);

		label_cleanup(octx);

		out_dbg_where(octx, &d->bits.func.code->where_cbrace);

		{
			char *end = out_dbg_func_end(decl_asm_spel(d));
			out_func_epilogue(octx, d->ref, &d->bits.func.code->where, end, section);
			free(end);
		}

		if(out_dump_retained(octx, d->spel))
			gen_had_error = 1;

		out_perfunc_teardown(octx);

	}else{
		asm_declare_decl_init(section, d);
	}

	if(cc1_target_details.as->supports_type_and_size)
		gen_type_and_size(section, d);
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
	const int via_got = decl_needs_GOTPLT(d);

	return out_new_lbl(
			octx,
			type_ptr_to(d->ref),
			decl_asm_spel(d),
			OUT_LBL_PIC | (via_got ? 0 : OUT_LBL_PICLOCAL));
}

static void gen_gasm(char *asm_str)
{
	struct section sec = SECTION_INIT(SECTION_TEXT); /* no option for global asm, always text */
	asm_out_section(&sec, "%s\n", asm_str);
}

static void gen_stringlits(dynmap *litmap)
{
	const stringlit *lit;
	size_t i;
	struct section sec = SECTION_INIT(SECTION_RODATA);

	for(i = 0; (lit = dynmap_value(stringlit *, litmap, i)); i++)
		if(lit->use_cnt > 0)
			asm_declare_stringlit(&sec, lit);
}

void gen_asm_emit_type(out_ctx *octx, type *ty)
{
	/* for types that aren't on variables (e.g. in exprs),
	 * that debug info may not find out about normally */
	if(cc1_gdebug == DEBUG_FULL && type_is_s_or_u(ty))
		out_dbg_emit_type(octx, ty);
}

static void infer_decl_section(decl *d, struct section *sec)
{
	const int is_code = !!type_is(d->ref, type_func);
	const int is_ro = is_code || type_is_const(d->ref);
	const enum section_flags flags = (is_code ? SECTION_FLAG_EXECUTABLE : 0) | (is_ro ? SECTION_FLAG_RO : 0);
	attribute *attr;

	if((attr = attribute_present(d, attr_section))){
		SECTION_FROM_NAME(sec, attr->bits.section, flags);
		return;
	}

	if(type_is(d->ref, type_func)){
		if(cc1_fopt.function_sections){
			SECTION_FROM_FUNCDECL(sec, decl_asm_spel(d), flags);
			return;
		}

		SECTION_FROM_BUILTIN(sec, SECTION_TEXT, flags);
		return;
	}

	if(cc1_fopt.data_sections){
		SECTION_FROM_DATADECL(sec, decl_asm_spel(d), flags);
		return;
	}

	/* prefer rodata over bss */
	if(type_is_const(d->ref)){
		if(FOPT_PIC(&cc1_fopt)
		&& d->bits.var.init.dinit
		&& decl_init_requires_relocation(d->bits.var.init.dinit))
		{
			SECTION_FROM_BUILTIN(sec, SECTION_RELRO, flags);
			return;
		}

		SECTION_FROM_BUILTIN(sec, SECTION_RODATA, flags);
		return;
	}

	/* d->bits.var.init.dinit may be null for extern decls */
	if(d->bits.var.init.dinit){
		if(DECL_INIT_COMPILER_GENERATED(d->bits.var.init)
		|| (cc1_fopt.zero_init_in_bss && decl_init_is_zero(d->bits.var.init.dinit)))
		{
			SECTION_FROM_BUILTIN(sec, SECTION_BSS, flags);
			return;
		}
	}

	SECTION_FROM_BUILTIN(sec, SECTION_DATA, flags);
}

void gen_asm_global_w_store(decl *d, int emit_tenatives, out_ctx *octx)
{
	struct section section;
	decl *old_decl;
	struct cc1_out_ctx *cc1_octx = *cc1_out_ctx(octx);
	int emitted_type = 0;
	const int attr_used_present = !!attribute_present(d, attr_used);
	int emit_visibility = 0;
	attribute *attr;

	/* in map? */
	if(cc1_octx && dynmap_exists(decl *, cc1_octx->generated_decls, d))
		return;

	/* add to map */
	cc1_octx = cc1_out_ctx_or_new(octx);
	if(!cc1_octx->generated_decls)
		cc1_octx->generated_decls = dynmap_new(decl *, /*ref*/NULL, decl_hash);
	(void)dynmap_set(decl *, int *, cc1_octx->generated_decls, d, (int *)NULL);

	if(!decl_asm_spel(d))
		return; /* struct A { ... }; */

	if(!cc1_octx->spel_to_fndecl){
		cc1_octx->spel_to_fndecl = dynmap_new(
				const char *, strcmp, dynmap_strhash);
	}
	(void)dynmap_set(
			const char *, decl *,
			cc1_octx->spel_to_fndecl,
			decl_asm_spel(d), d);

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

	old_decl = cc1_octx->current_decl;
	cc1_octx->current_decl = d;

	infer_decl_section(d, &section);
	asm_switch_section(&section);
	if(cc1_fopt.dump_decl_sections){
		int allocated;
		char *name = section_name(&section, &allocated);
		fprintf(stderr, "%s --> section \"%s\"\n", decl_to_str(d), name);
		if(allocated)
			free(name);
	}


	if(attribute_present(d, attr_weak)){
		asm_predeclare_weak(&section, d);
		emitted_type = 1;
	}
	if((attr = attribute_present(d, attr_alias))){
		assert(attr->type == attr_alias);
		assert(!decl_defined(d, 0));
		asm_declare_alias(&section, d, attr->bits.alias);
		emit_visibility = 1;
	}

	if(type_is(d->ref, type_func)){
		if(!decl_should_emit_code(d)){
			/* inline only gets extern emitted anyway */
			if(!emitted_type)
				asm_predeclare_extern(&section, d);

			goto out;
		}

		if(cc1_gdebug != DEBUG_OFF)
			out_dbg_emit_func(octx, d);
	}else{
		/* variable - if there's no init,
		 * it's tenative and not output
		 *
		 * unless we're told to emit tenatives, e.g. local scope
		 */
		if((!emit_tenatives && !d->bits.var.init.dinit) || !decl_should_emit_var(d)){
			if(!emitted_type)
				asm_predeclare_extern(&section, d);
			goto out;
		}

		if(cc1_gdebug == DEBUG_FULL)
			out_dbg_emit_global_var(octx, d);
	}

	if(attr_used_present)
		asm_predeclare_used(&section, d);

	if(!emitted_type && decl_linkage(d) == linkage_external)
		asm_predeclare_global(&section, d);

	gen_asm_global(&section, d, octx);
	emit_visibility = 1;

out:
	if(emit_visibility)
		asm_predeclare_visibility(&section, d);
	cc1_octx->current_decl = old_decl;
}

void gen_asm(
		symtable_global *globs,
		const char *fname, const char *compdir,
		struct out_dbg_filelist **pfilelist,
		const char *producer)
{
	decl **inits = NULL, **terms = NULL;
	decl **diter;
	struct symtable_gasm **iasm = globs->gasms;
	out_ctx *octx = out_ctx_new();

	*pfilelist = NULL;

	if(cc1_gdebug != DEBUG_OFF)
		out_dbg_begin(octx, &octx->dbg.file_head, fname, compdir, cc1_std, producer);

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

	gen_inits_terms(inits, terms, octx);
	dynarray_free(decl **, inits, NULL);
	dynarray_free(decl **, terms, NULL);

	if(cc1_gdebug != DEBUG_OFF){
		out_dbg_end(octx);

		*pfilelist = octx->dbg.file_head;
	}

	cc1_out_ctx_free(octx);
	out_ctx_end(octx);
}
