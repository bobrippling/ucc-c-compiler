#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "sym.h"
#include "fold_sym.h"
#include "../util/platform.h"
#include "pack.h"
#include "sue.h"
#include "out/out.h"


#define RW_TEST(var)                              \
						s->var == 0                           \
						&& s->decl->spel                      \
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
	const int this_start = current;
	int arg_space = 0;

	/*if(tab->typedefs){
		decl **i;
		for(i = tab->typedefs; *i; i++)
			fold_decl(*i, tab);
	}*/

	if(tab->decls){
		decl **diter;
		int arg_idx;

		arg_idx = 0;

		/* need to walk backwards for args */
		for(diter = tab->decls; *diter; diter++);

		for(diter--; diter >= tab->decls; diter--){
			sym *s = (*diter)->sym;

			if(s->type == sym_arg){
				s->offset = arg_idx++;
				s->decl->is_definition = 1; /* just for completeness */
			}
		}

		arg_space = arg_idx * platform_word_size();
		current += arg_space;

		for(diter = tab->decls; *diter; diter++){
			sym *s = (*diter)->sym;
			const int has_unused_attr = !!decl_has_attr(s->decl, attr_unused);

			if(s->type == sym_local){
				if(DECL_IS_FUNC(s->decl))
					continue;

				switch(s->decl->store){
					case store_default:
					case store_auto:
					{
						int siz = decl_size(s->decl, &s->decl->where);
						int align = type_ref_align(s->decl->ref, &s->decl->where);

						/* packing takes care of everything */
						pack_next(&current, NULL, siz, align);

						s->offset = current;

						/* static analysis on sym (only auto-vars) */
						if(!has_unused_attr && !s->decl->init)
							RW_WARN(WRITTEN, nwrites, "written to");
						break;
					}

					default:
						break;
				}
			}

			switch(s->type){
				case sym_arg:
				case sym_local: /* warn on unused args and locals */
				{
					const int unused = RW_TEST(nreads);

					if(unused){
						if(!has_unused_attr && (s->decl->store & STORE_MASK_STORE) != store_extern)
							RW_SHOW(READ, "read");
					}else if(has_unused_attr){
						warn_at(&s->decl->where, 1,
								"\"%s\" declared unused, but is used", s->decl->spel);
					}

					break;
				}

				case sym_global:
					break;
			}
		}
	}

	/* round current up to word size */
	current = pack_to_word(current);

	{
		symtable **tabi;
		int subtab_max = 0;

		for(tabi = tab->children; tabi && *tabi; tabi++){
			int this = symtab_fold(*tabi, current);
			if(this > subtab_max)
				subtab_max = this;
		}

		/* don't account the args in the space */
		tab->auto_total_size = current - this_start + subtab_max;
	}

	return tab->auto_total_size;
}
