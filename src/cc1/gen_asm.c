#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "macros.h"
#include "asm.h"
#include "../util/platform.h"
#include "sym.h"
#include "gen_asm.h"
#include "../util/util.h"
#include "const.h"

char *curfunc_lblfin; /* extern */

void gen_expr(expr *e, symtable *stab)
{
	e->f_gen(e, stab);
}

void gen_stmt(stmt *t)
{
	t->f_gen(t);
}

#ifdef FANCY_STACK_INIT
#warning FANCY_STACK_INIT broken
void gen_func_stack(decl *df, const int offset)
{
#define ITER_DECLS() \
		for(iter = df->func_code->symtab->decls; iter && *iter; iter++)

	int use_sub = 1;
	decl **iter;

	ITER_DECLS(){
		decl *d = *iter;
		if(decl_is_array(d) && d->init){
			use_sub = 0;
		}
	}

	if(use_sub){
		asm_temp(1, "sub rsp, %d", offset);
	}else{
		ITER_DECLS(){
			decl *d = *iter;
			if(decl_is_array(d) && d->init){
			}else{
			}
		}
	}
}
#else
#  define gen_func_stack(df, offset) asm_temp(1, "sub rsp, %d", offset)
#endif

void asm_extern(decl *d)
{
	asm_tempf(cc_out[SECTION_BSS], 0, "extern %s", d->spel_asm);
}

void gen_asm_global(decl *d)
{
	if(!d->is_definition)
		return;

	if(decl_attr_present(d->attr, attr_section))
		ICW("%s: TODO: section attribute \"%s\" on %s",
				where_str(&d->attr->where), d->attr->attr_extra.section, d->spel_asm);

	/* order of the if matters */
	if(d->func_code){
		const int offset = d->func_code->symtab->auto_total_size;

		asm_label(d->spel_asm);
		asm_temp(1, "push rbp");
		asm_temp(1, "mov rbp, rsp");

		curfunc_lblfin = asm_label_code(d->spel_asm);

		if(offset)
			gen_func_stack(d, offset);

		gen_stmt(d->func_code);

		asm_label(curfunc_lblfin);
		if(offset)
			asm_temp(1, "add rsp, %d", offset);

		asm_temp(1, "leave");
		asm_temp(1, "ret");
		free(curfunc_lblfin);

	}else if(d->arrayinit){
		asm_declare_array(SECTION_DATA, d->arrayinit->label, d->arrayinit);

	}else if(d->init && !const_expr_is_zero(d->init)){
		asm_declare_single(cc_out[SECTION_DATA], d);

	}else if(d->type->store == store_extern){
		asm_extern(d);

	}else{
		/* always resb, since we use decl_size() */
		asm_tempf(cc_out[SECTION_BSS], 0, "%s resb %d", d->spel_asm, decl_size(d));
	}
}

void gen_asm(symtable *globs)
{
	decl **diter;

	for(diter = globs->decls; diter && *diter; diter++){
		decl *d = *diter;

		/* inline_only aren't currently inlined */
		if(!d->is_definition)
			continue;

		if(d->inline_only){
			/* emit an extern for it anyway */
			asm_extern(d);
			continue;
		}

		switch(d->type->store){
			case store_auto:
			case store_register:
			case store_typedef:
				ICE("%s storage on global", type_store_to_str(d->type->store));

			case store_static:
				break;

			case store_extern:
				if(!decl_is_func(d) || !d->func_code)
					break;
				/* else extern func with definition */

			case store_default:
				asm_temp(0, "global %s", d->spel_asm);
		}

		gen_asm_global(d);
	}
}
