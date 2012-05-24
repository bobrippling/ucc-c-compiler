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

void gen_asm_extern(decl *d)
{
	asm_tempf(cc_out[SECTION_BSS], 0, "extern %s", decl_spel(d));
}

void gen_asm_global(decl *d)
{
	if(!d->is_definition)
		return;

	if(decl_attr_present(d->attr, attr_section))
		ICW("%s: TODO: section attribute \"%s\" on %s",
				where_str(&d->attr->where), d->attr->attr_extra.section, d->spel);

	if(d->type->store == store_extern){
		gen_asm_extern(d);

	}else if(d->func_code){
		const int offset = d->func_code->symtab->auto_total_size;

		asm_label(decl_spel(d));
		asm_temp(1, "push rbp");
		asm_temp(1, "mov rbp, rsp");

		curfunc_lblfin = asm_label_code(decl_spel(d));

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

	}else{
		/* always resb, since we use decl_size() */
		asm_tempf(cc_out[SECTION_BSS], 0, "%s resb %d", decl_spel(d), decl_size(d));
	}
}

void gen_asm(symtable *globs)
{
	decl **diter;
	for(diter = globs->decls; diter && *diter; diter++){
		decl *d = *diter;

		if(!d->is_definition)
			continue;

		if(!type_store_static_or_extern(d->type->store))
			asm_temp(0, "global %s", decl_spel(d));

		gen_asm_global(d);
	}
}
