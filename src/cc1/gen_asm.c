#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "../util/alloc.h"
#include "data_structs.h"
#include "cc1.h"
#include "macros.h"
#include "../util/platform.h"
#include "sym.h"
#include "gen_asm.h"
#include "../util/util.h"
#include "const.h"
#include "tree.h"
#include "out/out.h"
#include "out/lbl.h"
#include "out/asm.h"
#include "out/basic_block.h"
#include "gen_style.h"

char *curfunc_lblfin; /* extern */

void gen_expr(expr *e, basic_blk *bb)
{
	consty k;

	const_fold(e, &k);

	if(k.type == CONST_NUM){
		/* -O0 skips this? */
		if(cc1_backend == BACKEND_ASM)
			out_push_num(e->tree_type, &k.bits.num); /* bb is unchanged */
		else
			stylef("%" NUMERIC_FMT_D, k.bits.num.val.i);
	}else{
		if(cc1_gdebug)
			out_comment("at %s", where_str(&e->where));

		EOF_WHERE(&e->where, bb = e->f_gen(e, bb));
	}

	return bb;
}

void lea_expr(expr *e)
{
	UCC_ASSERT(e->f_lea,
			"invalid store expression expr-%s (no f_store())", e->f_str());

	e->f_lea(e);
}

struct basic_blk *gen_stmt(stmt *t, struct basic_blk *startblk)
{
	struct basic_blk *out;
	EOF_WHERE(&t->where, out = t->f_gen(t, startblk));
	return out;
}

static void assign_arg_offsets(decl **decls, int const offsets[])
{
	unsigned i, j;

	for(i = j = 0; decls && decls[i]; i++){
		sym *s = decls[i]->sym;

		if(s && s->type == sym_arg){
			if(fopt_mode & FOPT_VERBOSE_ASM)
				out_comment("%s @ offset %d", s->decl->spel, offsets[j]);

			s->loc.arg_offset = offsets[j++];
		}
	}
}

void gen_asm_global(decl *d)
{
	decl_attr *sec;

	if((sec = decl_attr_present(d, attr_section))){
		ICW("%s: TODO: section attribute \"%s\" on %s",
				where_str(&sec->where),
				sec->bits.section, d->spel);
	}

	/* order of the if matters */
	if(DECL_IS_FUNC(d) || type_ref_is(d->ref, type_ref_block)){
		/* check .func_code, since it could be a block */
		int nargs = 0, is_vari;
		decl **aiter;
		const char *sp;
		int *offsets;
		symtable *arg_symtab;
		struct basic_blk *bb_start, *bb_end;

		if(!d->func_code)
			return;

		arg_symtab = DECL_FUNC_ARG_SYMTAB(d);
		for(aiter = arg_symtab->decls; aiter && *aiter; aiter++)
			if((*aiter)->sym->type == sym_arg)
				nargs++;

		offsets = nargs ? umalloc(nargs * sizeof *offsets) : NULL;

		sp = decl_asm_spel(d);

		out_label(sp);

		bb_start = out_func_prologue(d->ref,
				d->func_code->symtab->auto_total_size,
				nargs,
				is_vari = type_ref_is_variadic_func(d->ref),
				offsets);

		assign_arg_offsets(arg_symtab->decls, offsets);

		curfunc_lblfin = out_label_code(sp);

		bb_end = gen_stmt(d->func_code, bb_start);

		out_label(curfunc_lblfin);

		out_func_epilogue(d->ref, bb_end);

		free(curfunc_lblfin);
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

void gen_asm(symtable_global *globs)
{
	decl **diter;
	struct symtable_gasm **iasm = globs->gasms;

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

		if(DECL_IS_FUNC(d)){
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

			if(!d->func_code){
				asm_predeclare_extern(d);
				continue;
			}
		}else{
			/* variable - if there's no init,
			 * it's tenative and not output */
			if(!d->init){
				asm_predeclare_extern(d);
				continue;
			}
		}

		if((d->store & STORE_MASK_STORE) != store_static)
			asm_predeclare_global(d);
		gen_asm_global(d);

		UCC_ASSERT(out_vcount() == 0, "non empty vstack after global gen");
	}

	for(; iasm && *iasm; ++iasm)
		gen_gasm((*iasm)->asm_str);
}
