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

void gen_style_dinit(decl_init *di)
{
	/* TODO */
}

void gen_style_decl(decl *d)
{
	stylef("%s", decl_to_str(d));

	if(d->func_code){
		gen_stmt(d->func_code);
		return;
	}

	if(d->init){
		stylef(" = ");
		gen_style_dinit(d->init);
	}
	stylef(";\n");
}

void gen_style(symtable_global *stab)
{
	decl **i;
	for(i = stab->stab.decls; i && *i; i++)
		gen_style_decl(*i);
}
