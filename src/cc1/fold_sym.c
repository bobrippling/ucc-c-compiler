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


#define RW_TEST(var)                              \
						s->var == 0                           \
						&& s->decl->spel                      \
						&& (s->decl->store & STORE_MASK_STORE)\
						        != store_typedef              \
						&& !DECL_IS_ARRAY(s->decl)            \
						&& !DECL_IS_FUNC(s->decl)             \
						&& !DECL_IS_S_OR_U(s->decl)

#define RW_SHOW(w, str)                           \
					cc1_warn_at(&s->decl->where, 0, 1,      \
							WARN_SYM_NEVER_ ## w,               \
							"\"%s\" never " str,                \
							s->decl->spel);                     \

#define RW_WARN(w, var, str)                            \
						do{                                         \
							if(RW_TEST(var)){                         \
								RW_SHOW(w, str)                         \
								s->var++;                               \
							}                                         \
						}while(0)

int symtab_fold(symtable *tab, int current)
{
#define LOCAL_SCOPE !!(tab->parent)
	const int this_start = current;
	int arg_space = 0;
	char wbuf[WHERE_BUF_SIZ];

	decl **all_decls = NULL;

	if(tab->decls){
		decl **diter;
		int arg_idx;

		arg_idx = 0;

		/* need to walk backwards for args */
		for(diter = tab->decls; *diter; diter++);

		for(diter--; diter >= tab->decls; diter--){
			decl *d = *diter;
			sym *s = d->sym;

			fold_decl(d, tab);

			if(s->type == sym_arg)
				s->offset = arg_idx++;
		}

		for(diter = tab->decls; *diter; diter++){
			decl *d = *diter;
			sym *s = d->sym;
			const int has_unused_attr = !!decl_has_attr(d, attr_unused);

			if(d->spel)
				dynarray_add(&all_decls, d);

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

							/* static analysis on sym (only auto-vars) */
							if(!has_unused_attr && !d->init)
								RW_WARN(WRITTEN, nwrites, "written to");
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
				{
					const int unused = RW_TEST(nreads);

					if(unused){
						if(!has_unused_attr && (d->store & STORE_MASK_STORE) != store_extern)
							RW_SHOW(READ, "read");
					}else if(has_unused_attr){
						warn_at(&d->where, 1,
								"\"%s\" declared unused, but is used", d->spel);
					}

					break;
				}

				case sym_global:
					break;
			}

			switch((enum decl_storage)(d->store & STORE_MASK_STORE)){
				case store_register:
				case store_extern:
					break;
				default:
					if(s->type != sym_global){
						/* allow anonymous decls to have .spel_asm */
						if(d->spel && d->spel_asm){
							DIE_AT(&d->where,
									"asm() rename on non-register non-global variable \"%s\" (%s)",
									d->spel, d->spel_asm);
						}
					}
			}
		}
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
			int is_func;

			/* we allow multiple function declarations,
			 * and multiple declarations at global scope,
			 * but not definitions
			 */
			if(!strcmp(a->spel, b->spel)){
				if(LOCAL_SCOPE){
					clash = "clashing";
				}else{
					if((is_func = !!DECL_IS_FUNC(a)) != !!DECL_IS_FUNC(b)){
						clash = "mismatching";
					}else if(is_func){
						if(a->func_code && b->func_code)
							clash = "duplicate";
					}
					if(!clash && !decl_equal(a, b, DECL_CMP_EXACT_MATCH))
						clash = "mismatching";
				}
			}

			if(clash){
				/* XXX: note */
				DIE_AT(&a->where,
						"%s definitions of \"%s\"\n"
						"%s: note: other definition",
						clash, a->spel,
						where_str_r(wbuf, &b->where));
			}
			}


		dynarray_free(decl **, &all_decls, NULL);
	}

	{
		symtable **tabi;
		int subtab_max = 0;

		for(tabi = tab->children; tabi && *tabi; tabi++){
			int this = symtab_fold(*tabi, current);
			if(this > subtab_max)
				subtab_max = this;
		}

		/* don't account the args in the space,
		 * just use for offsetting them
		 */
		tab->auto_total_size = current - this_start + subtab_max - arg_space;
	}

	return tab->auto_total_size;
#undef LOCAL_SCOPE
}
