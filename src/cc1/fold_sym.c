#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "sym.h"
#include "fold_sym.h"
#include "../util/platform.h"

int symtab_fold(symtable *tab, int current)
{
	const int this_start = current;

	if(tab->decls){
		const int word_size = platform_word_size();
		decl **diter;
		int arg_offset;

		arg_offset = 0;

		/* need to walk backwards for args */
		for(diter = tab->decls; *diter; diter++);

		for(diter--; diter >= tab->decls; diter--){
			sym *s = (*diter)->sym;
			/*enum type_primitive last = type_int; TODO: packing */

			if(s->type == sym_local){
				switch(s->decl->type->store){
					case store_auto:
					case store_default:
						if(!decl_is_func(s->decl)){
						int siz = decl_size(s->decl);

						if(siz <= word_size)
							s->offset = current;
						else
							s->offset = current + siz - word_size; /* an array and structs start at the bottom */

						/* need to increase by a multiple of word_size */
						if(siz % word_size)
							siz += word_size - siz % word_size;
						current += siz;

#define RW_WARN(w, var, str)                            \
						do{                                         \
							if(s->var == 0                            \
									&& !s->decl->internal                 \
									&& !decl_has_array(s->decl)           \
									&& !decl_is_func(s->decl)             \
									&& !decl_is_struct_or_union(s->decl)) \
							{                                         \
								cc1_warn_at(&s->decl->where, 0,         \
										WARN_SYM_NEVER_ ## w,               \
										"\"%s\" never " str,                \
										s->decl->spel);                \
								s->var++;                               \
							}                                         \
						}while(0)


						/* static analysis on sym (only auto-vars) */
						RW_WARN(WRITTEN, nwrites, "written to");
					}

					default:
						break;
				}

			}else if(s->type == sym_arg){
				s->offset = arg_offset;
				arg_offset += word_size;
				s->decl->is_definition = 1; /* just for completeness */

			}

			switch(s->type){
				case sym_arg:
				case sym_local:
					/* warn on unused args and locals */
					if(!decl_attr_present(s->decl->attr, attr_unused))
						RW_WARN(READ, nreads, "read");

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

		tab->auto_total_size = current - this_start + subtab_max;
	}

	return tab->auto_total_size;
}
