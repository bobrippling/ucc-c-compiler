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
						&& !decl_has_array(s->decl)           \
						&& !decl_is_func(s->decl)             \
						&& !decl_is_struct_or_union(s->decl)

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
			const int has_unused_attr = decl_attr_present(s->decl->attr, attr_unused);

			if(s->type == sym_local){
				switch(s->decl->type->store){
					case store_default:
					case store_auto:
					{
						int siz = decl_size(s->decl);
						int align;
						int this;

						if(decl_is_struct_or_union(s->decl))
							/* safe - can't have an instance without a ->sue */
							align = s->decl->type->sue->align;
						else
							align = siz;

						pack_next(&current, &this, siz, align); /* an array and structs start at the bottom */

						s->offset = this;

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
						if(!has_unused_attr && s->decl->type->store != store_extern)
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
}
