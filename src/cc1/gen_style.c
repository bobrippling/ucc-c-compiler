#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "expr.h"
#include "stmt.h"
#include "cc1.h"
#include "sym.h"
#include "gen_style.h"

void gen_stmt_style(stmt *s)
{ (void)s; /* TODO */ }

void gen_style_global(decl *d)
{
	fprintf(cc1_out, "global decl %s\n", d->spel);
	if(d->func_code)
		gen_stmt_style(d->func_code);
}

void gen_style(symtable_global *stab)
{
	decl **diter;
	for(diter = stab->symtab.decls; diter && *diter; diter++){
		decl *d = *diter;

		gen_style_global(d);
	}
}
