#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "cc1.h"
#include "sym.h"
#include "sym_fold.h"
#include "../util/platform.h"

int symtab_fold(symtable *tab, int current)
{
	const int start = current;

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
				s->offset = current;
				current += decl_size(s->decl);

			}else if(s->type == sym_arg){
				s->offset = arg_offset;
				arg_offset += word_size;

			}
		}
	}


	{
		symtable **tabi;
		int subtab_size = 0;

		for(tabi = tab->children; tabi && *tabi; tabi++)
			subtab_size += symtab_fold(*tabi, current);

		current += subtab_size;
	}

	tab->auto_total_size = current;

	return current;
}
