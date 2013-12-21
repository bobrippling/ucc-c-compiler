#include <stdio.h>
#include <stdarg.h>

#include "../util/util.h"
#include "../util/dynarray.h"

#include "data_structs.h"
#include "expr.h"
#include "stmt.h"
#include "cc1.h"
#include "sym.h"
#include "gen_style.h"
#include "gen_asm.h" /* FIXME: gen_stmt/_expr should be in gen.h */
#include "decl_init.h"
#include "out/asm.h" /* cc*_out */

void stylef(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	asm_out_sectionv(SECTION_TEXT, fmt, l);
	va_end(l);
}

void gen_style_dinit(decl_init *di)
{
	switch(di->type){
		case decl_init_scalar:
			gen_expr(di->bits.expr);
			break;

		case decl_init_copy:
			ICE("copy in gen");
			break;

		case decl_init_brace:
		{
			decl_init *s;
			int i;

			stylef("{");

			for(i = 0; (s = di->bits.ar.inits[i]); i++){
				if(s == DYNARRAY_NULL){
					stylef("0");
				}else if(s->type == decl_init_copy){
					stylef("<copy from store %ld>",
							(long)DECL_INIT_COPY_IDX(s, di));
				}else{
					gen_style_dinit(s);
				}

				if(di->bits.ar.inits[i + 1])
					stylef(",\n");
			}

			stylef("}");
		}
	}
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

void gen_style(symtable_global *stab, const char *fname)
{
	decl **i;

	(void)fname;

	for(i = stab->stab.decls; i && *i; i++)
		gen_style_decl(*i);
}
