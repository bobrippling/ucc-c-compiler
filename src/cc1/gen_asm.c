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
#include "data_store.h"

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
void gen_func_stack(decl *df, int offset)
{
#define ITER_DECLS(i) \
		for(i = df->func_code->symtab->decls; i && *i; i++)

	int clever = 0;
	decl **iter;

	ITER_DECLS(iter)
		if((*iter)->init){
			clever = 1;
			break;
		}

	if(clever){
		/*const int old_offset = offset;
		offset = 0;*/

		ITER_DECLS(iter){
			decl *d = *iter;
			if(d->init && d->init->type != decl_init_scalar){
				ICW("TODO: stack gen or expr for %s init", decl_to_str(d));
			}
		}
	}

	asm_temp(1, "sub rsp, %d", offset);
}
#else
#  define gen_func_stack(df, offset) asm_temp(1, "sub rsp, %d", offset)
#endif

void gen_asm_extern(decl *d)
{
	asm_tempf(cc_out[SECTION_BSS], 0, "extern %s", d->spel);
}

void gen_asm_global(decl *d)
{
	if(!d->is_definition)
		return;

	if(d->builtin)
		return;

	if(decl_attr_present(d->attr, attr_section))
		ICW("%s: TODO: section attribute \"%s\" on %s",
				where_str(&d->attr->where), d->attr->attr_extra.section, d->spel);

	/* order of the if matters */
	if(d->func_code){
		const int offset = d->func_code->symtab->auto_total_size;

		asm_label(d->spel);
		asm_temp(1, "push rbp");
		asm_temp(1, "mov rbp, rsp");

		curfunc_lblfin = asm_label_code(d->spel);

		if(offset)
			gen_func_stack(d, offset);

		gen_stmt(d->func_code);

		asm_label(curfunc_lblfin);
		if(offset)
			asm_temp(1, "add rsp, %d", offset);

		asm_temp(1, "leave");
		asm_temp(1, "ret");
		free(curfunc_lblfin);

	}else{
		/* takes care of static, extern, etc */
		asm_declare(cc_out[SECTION_DATA], d);
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
			gen_asm_extern(d);
			continue;
		}

		switch(d->type->store){
			case store_auto:
			case store_register:
			case store_typedef:
				ICE("%s storage on global %s",
						type_store_to_str(d->type->store),
						decl_to_str(d));

			case store_static:
				break;

			case store_extern:
				if(!decl_is_func(d) || !d->func_code)
					break;
				/* else extern func with definition */

			case store_default:
				asm_temp(0, "global %s", d->spel);
		}

		gen_asm_global(d);
	}
}
