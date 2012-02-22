#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
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

			if(s->type == sym_local && (s->decl->type->spec & (spec_extern | spec_static)) == 0){
				int siz = decl_size(s->decl);

				if(decl_has_array(s->decl))
					s->offset = current + siz; /* an array starts at the bottom */
				else
					s->offset = current;

				/* need to increase by a multiple of word_size */
				if(siz % word_size)
					siz += word_size - siz % word_size;
				current += siz;

			}else if(s->type == sym_arg){
				s->offset = arg_offset;
				arg_offset += word_size;

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
