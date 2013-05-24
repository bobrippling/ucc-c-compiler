#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "data_structs.h"
#include "expr.h"
#include "stmt.h"
#include "cc1.h"
#include "sym.h"
#include "gen_style.h"
#include "gen_asm.h" /* FIXME: gen_stmt/_expr should be in gen.h */

void gen_style_decl(decl *d)
{
	stylef("%s", decl_to_str(d));
}

void gen_style_global(decl *d)
{
	gen_style_decl(d);
	if(d->func_code){
		gen_stmt(d->func_code);
	}else{
		stylef(";\n");
	}
}

void gen_style(symtable_global *stab)
{
	decl **diter;
	for(diter = stab->stab.decls; diter && *diter; diter++){
		decl *d = *diter;

		gen_style_global(d);
	}
}
