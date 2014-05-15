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
#include "type_is.h"
#include "gen_asm.h"
#include "out/out.h"
#include "out/lbl.h"
#include "out/asm.h"
#include "gen_style.h"
#include "out/dbg.h"

void IGNORE_PRINTGEN(out_val *v)
{
	(void)v;
}

out_val *gen_expr(expr *e, out_ctx *octx)
{
	consty k;

	/* always const_fold functions, i.e. builtins */
	if(expr_kind(e, funcall) || fopt_mode & FOPT_CONST_FOLD)
		const_fold(e, &k);
	else
		k.type = CONST_NO;

	out_dbg_where(&e->where);

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

out_val *lea_expr(expr *e, out_ctx *octx)
{
	char buf[WHERE_BUF_SIZ];

	UCC_ASSERT(e->f_lea,
			"invalid store expression expr-%s @ %s (no f_lea())",
			e->f_str(), where_str_r(buf, &e->where));

	out_dbg_where(&e->where);

	return e->f_lea(e, octx);
}

void gen_stmt(stmt *t, out_ctx *octx)
{
	out_dbg_where(&t->where);

	t->f_gen(t, octx);
}

static void assign_arg_offsets(
		out_ctx *octx,
		decl **decls, int const offsets[])
{
	unsigned i, j;

	for(i = j = 0; decls && decls[i]; i++){
		sym *s = decls[i]->sym;

		if(s && s->type == sym_arg){
			if(fopt_mode & FOPT_VERBOSE_ASM)
				out_comment(octx, "%s @ offset %d", s->decl->spel, offsets[j]);

			s->loc.arg_offset = offsets[j++];
		}
	}
}

void gen_asm_global(decl *d, out_ctx *octx)
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
		int *offsets;
		symtable *arg_symtab;

		if(!d->bits.func.code)
			return;

		out_dbg_where(&d->where);

		arg_symtab = DECL_FUNC_ARG_SYMTAB(d);
		for(aiter = arg_symtab->decls; aiter && *aiter; aiter++)
			if((*aiter)->sym->type == sym_arg)
				nargs++;

		offsets = nargs ? umalloc(nargs * sizeof *offsets) : NULL;

		sp = decl_asm_spel(d);

		out_func_prologue(octx, sp, d->ref,
				d->bits.func.code->symtab->auto_total_size,
				nargs,
				is_vari = type_is_variadic_func(d->ref),
				offsets, &d->bits.func.var_offset);

		assign_arg_offsets(octx, arg_symtab->decls, offsets);

		gen_stmt(d->bits.func.code, octx);

		out_dump_retained(octx, d->spel);

		out_dbg_where(&d->bits.func.code->where_cbrace);

		{
			char *end = out_dbg_func_end(decl_asm_spel(d));
			out_func_epilogue(octx, d->ref, end);
			free(end);
		}

		free(offsets);

	}else{
		/* asm takes care of .bss vs .data, etc */
		asm_declare_decl_init(SECTION_DATA, d);
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

void gen_asm(symtable_global *globs, const char *fname, const char *compdir)
{
	decl **diter;
	struct symtable_gasm **iasm = globs->gasms;
	out_ctx *octx = out_ctx_new();

	for(diter = globs->stab.decls; diter && *diter; diter++){
		decl *d = *diter;

		while(iasm && d == (*iasm)->before){
			gen_gasm((*iasm)->asm_str);

			if(!*++iasm)
				iasm = NULL;
		}

		switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
			case store_inline:
			case store_auto:
			case store_register:
				ICE("%s storage on global %s",
						decl_store_to_str(d->store),
						decl_to_str(d));

			case store_typedef:
				continue;

			case store_extern:
			case store_default:
			case store_static:
				break;
		}

		if(type_is(d->ref, type_func)){
			if(d->store & store_inline){
				/*
				 * inline semantics
				 *
				 * "" = inline only
				 * "static" = code emitted, decl is static
				 * "extern" mentioned, or "inline" not mentioned = code emitted, decl is extern
				 */
				if((d->store & STORE_MASK_STORE) == store_default){
					/* inline only - emit an extern for it anyway */
					asm_predeclare_extern(d);
					continue;
				}
			}

			if(!d->bits.func.code){
				asm_predeclare_extern(d);
				continue;
			}
		}else{
			/* variable - if there's no init,
			 * it's tenative and not output */
			if(!d->bits.var.init){
				asm_predeclare_extern(d);
				continue;
			}
		}

		if((d->store & STORE_MASK_STORE) != store_static)
			asm_predeclare_global(d);
		gen_asm_global(d, octx);
	}

	for(; iasm && *iasm; ++iasm)
		gen_gasm((*iasm)->asm_str);

	gen_stringlits(globs->literals);

	if(cc1_gdebug && globs->stab.decls)
		out_dbginfo(globs, fname, compdir);

	out_ctx_end(octx);
}
