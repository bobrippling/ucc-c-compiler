#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "sym.h"
#include "fold_sym.h"
#include "../util/platform.h"
#include "../util/dynarray.h"
#include "pack.h"
#include "sue.h"
#include "out/out.h"
#include "fold.h"
#include "fold_sue.h"
#include "decl_init.h"
#include "out/lbl.h"


#define RW_TEST(var)                                 \
						sym->var == 0                            \
						&& sym->decl->spel                       \
						&& (sym->decl->store & STORE_MASK_STORE) \
						        != store_typedef                 \
						&& !DECL_IS_ARRAY(sym->decl)             \
						&& !DECL_IS_FUNC(sym->decl)              \
						&& !DECL_IS_S_OR_U(sym->decl)

#define RW_SHOW(w, str)                           \
					cc1_warn_at(&sym->decl->where, 0, 1,    \
							WARN_SYM_NEVER_ ## w,               \
							"\"%s\" never " str,                \
							sym->decl->spel);                   \

#define RW_WARN(w, var, str)      \
						do{                   \
							if(RW_TEST(var)){   \
								RW_SHOW(w, str)   \
								sym->var++;       \
							}                   \
						}while(0)

void symtab_fold_decls(symtable *tab)
{
#define IS_LOCAL_SCOPE !!(tab->parent)
	decl **all_decls = NULL;
	decl **diter;

	for(diter = tab->decls; diter && *diter; diter++){
		decl *d = *diter;
		sym *const sym = d->sym;
		const int has_unused_attr = !!decl_attr_present(d, attr_unused);

		fold_decl(d, tab);

		if(d->spel)
			dynarray_add(&all_decls, d);

		switch(sym->type){
			case sym_local:
			{
				/* arg + local checks */
				const int unused = RW_TEST(nreads);

				switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
					case store_register:
					case store_default:
					case store_auto:
					case store_static:
						/* static analysis on sym */
						if(!has_unused_attr && !d->init)
							RW_WARN(WRITTEN, nwrites, "written to");
						break;
					case store_extern:
					case store_typedef:
					case store_inline:
						break;
				}
				/* fall */

				if(unused){
					if(!has_unused_attr && (d->store & STORE_MASK_STORE) != store_extern)
						RW_SHOW(READ, "read");
				}else if(has_unused_attr){
					warn_at(&d->where, 1,
							"\"%s\" declared unused, but is used", d->spel);
				}

			case sym_arg:
				/* asm rename checks */
				switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
					case store_register:
					case store_extern:
					case store_static:
						break;
					default:
						/* allow anonymous decls to have .spel_asm */
						if(d->spel && d->spel_asm){
							DIE_AT(&d->where,
									"asm() rename on non-register non-global variable \"%s\" (%s)",
									d->spel, d->spel_asm);
						}
				}
			}

			case sym_global:
				break;
		} /* sym switch */
	}


	if(all_decls){
		/* check_clashes */
		decl **di;

		qsort(all_decls,
				dynarray_count(all_decls),
				sizeof *all_decls,
				(int (*)(const void *, const void *))decl_sort_cmp);

		for(di = all_decls; di[1]; di++){
			decl *a = di[0], *b = di[1];
			char *clash = NULL;

			/* we allow multiple function declarations,
			 * and multiple declarations at global scope,
			 * but not definitions
			 */
			if(!strcmp(a->spel, b->spel)){
				const int a_func = !!DECL_IS_FUNC(a);

				if(!!DECL_IS_FUNC(b) != a_func
				|| !decl_equal(a, b,
					DECL_CMP_EXACT_MATCH | DECL_CMP_ALLOW_TENATIVE_ARRAY))
				{
					clash = "mismatching";
				}else{
					if(IS_LOCAL_SCOPE){
						/* allow multiple functions or multiple externs */
						if(a_func){
							/* fine - we know they're equal */
						}else if((a->store & STORE_MASK_STORE) == store_extern
								  && (b->store & STORE_MASK_STORE) == store_extern){
							/* both are extern declarations */
						}else{
							clash = "extern/non-extern";
						}
					}else if(a_func && a->func_code && b->func_code){
						clash = "duplicate";
					}
				}
			}

			if(clash){
				/* XXX: note */
				char wbuf[WHERE_BUF_SIZ];

				DIE_AT(&a->where,
						"%s definitions of \"%s\"\n"
						"%s: note: other definition",
						clash, a->spel,
						where_str_r(wbuf, &b->where));
			}
		}


		dynarray_free(decl **, &all_decls, NULL);
	}
#undef IS_LOCAL_SCOPE
}

unsigned symtab_layout_decls(symtable *tab, unsigned current)
{
	const unsigned this_start = current;
	int arg_space = 0;

	if(tab->decls){
		decl **diter;
		int arg_idx = 0;

		/* need to walk backwards for args */
		for(diter = tab->decls; *diter; diter++);

		for(diter--; diter >= tab->decls; diter--){
			sym *s = (*diter)->sym;

			if(s->type == sym_arg)
				s->offset = arg_idx++;
		}

		for(diter = tab->decls; *diter; diter++){
			decl *d = *diter;
			sym *s = d->sym;

			switch(s->type){
				case sym_local: /* warn on unused args and locals */
					if(DECL_IS_FUNC(d))
						continue;

					switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
							/* for now, we allocate stack space for register vars */
						case store_register:
						case store_default:
						case store_auto:
						{
							unsigned siz = decl_size(s->decl);
							unsigned align = decl_align(s->decl);

							/* align greater than size - we increase
							 * size so it can be aligned to `align'
							 */
							if(align > siz)
								siz = pack_to_align(siz, align);

							/* packing takes care of everything */
							pack_next(&current, NULL, siz, align);
							s->offset = current;
							break;
						}

						case store_static:
						case store_extern:
						case store_typedef:
							break;
						case store_inline:
							ICE("%s store", decl_store_to_str(d->store));
					}
					/* fall */

				case sym_arg:
				case sym_global:
					break;
			}
		}
	}

	{
		symtable **tabi;
		unsigned subtab_max = 0;

		for(tabi = tab->children; tabi && *tabi; tabi++){
			unsigned this = symtab_layout_decls(*tabi, current);
			if(this > subtab_max)
				subtab_max = this;
		}

		/* don't account the args in the space,
		 * just use for offsetting them
		 */
		tab->auto_total_size = current - this_start + subtab_max - arg_space;
	}

	return tab->auto_total_size;
}

void symtab_fold_sues(symtable *stab)
{
	struct_union_enum_st **sit;

	for(sit = stab->sues; sit && *sit; sit++)
		fold_sue(*sit, stab);
}

void symtab_make_syms_and_inits(
		symtable *stab, stmt **pinit_code)
{
#define inits (*pinit_code)
	/* this is called from wherever we can define a
	 * struct/union/enum,
	 * e.g. a code-block (explicit or implicit),
	 *      global scope
	 * and an if/switch/while statement: if((struct A { int i; } *)0)...
	 * an argument list/type_ref::func: f(struct A { int i, j; } *p, ...)
	 */

	unsigned di;
	decl *d;

	if(stab->folded)
		return;
	stab->folded = 1;

	/* sub-calls may change the array, must iterate with an index */
	for(di = 0; stab->decls && (d = stab->decls[di]); di++){
		if(stab->parent){
			if(d->func_code)
				DIE_AT(&d->func_code->where, "can't nest functions (%s)", d->spel);
			else if(DECL_IS_FUNC(d) && (d->store & STORE_MASK_STORE) == store_static)
				DIE_AT(&d->where, "block-scoped function cannot have static storage");
		}

		/* must be before fold*, since sym lookups are done */
		if(d->sym){
			/* arg */
			UCC_ASSERT(d->sym->type != sym_local || !d->spel /* anon sym, e.g. strk */,
					"sym (type %d) \"%s\" given symbol too early",
					d->sym->type, d->spel);
		}else{
			d->sym = sym_new(d,
					!stab->parent || decl_store_static_or_extern(d->store) ?
					sym_global :
					sym_local);
		}

		/* don't generate for anonymous symbols
		 * they're done elsewhere (e.g. compound literals)
		 */
		if(d->init && d->spel && pinit_code){
			/* this creates the below s->inits array */
			if((d->store & STORE_MASK_STORE) == store_static){
				fold_decl_global_init(d, stab);
			}else{
				EOF_WHERE(&d->where,
						if(!inits)
							inits = stmt_new_wrapper(code, symtab_new(stab));

						decl_init_brace_up_fold(d, inits->symtab);
						decl_init_create_assignments_base(d->init,
							d->ref, expr_new_identifier(d->spel),
							inits);
					);
				/* folded elsewhere */
			}
		}

		/* check static decls
		 * -> doesn't need to be after fold since we change .spel_asm
		 *
		 * don't for anonymous symbols, they're referenced via other means
		 */
		if(curdecl_func){
			if((d->store & STORE_MASK_STORE) == store_static && d->spel)
				d->spel_asm = out_label_static_local(curdecl_func->spel, d->spel);
		}
	}
#undef inits
}
