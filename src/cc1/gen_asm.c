#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"
#include "data_structs.h"
#include "cc1.h"
#include "macros.h"
#include "asm_out.h"
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
			break;
		}
	}

	if(use_sub){
		asm_output_new(asm_out_type_sub,
				asm_operand_new_reg(NULL, ASM_REG_SP),
				asm_operand_new_val(offset));
	}else{
		for(iter = df->func_code->symtab->decls; iter && *iter; iter++){
			decl *d = *iter;
			if(decl_is_array(d) && d->init){
				use_sub = 0;
			}else{

			}
		}
	}
}
#else
#  define gen_func_stack(df, offset)             \
			asm_output_new(asm_out_type_sub,           \
					asm_operand_new_reg(NULL, ASM_REG_SP), \
					asm_operand_new_val(offset))
#endif

void gen_asm_global(decl *d)
{
	if(decl_attr_present(d->attr, attr_section))
		ICW("%s: TODO: section attribute \"%s\" on %s",
				where_str(&d->attr->where), d->attr->attr_extra.section, d->spel);

	if(d->type->store == store_extern){
		/* should be fine... */
		asm_out_section(SECTION_BSS, "extern %s", d->spel);
		return;
	}

	if(d->func_code){
		const int offset = d->func_code->symtab->auto_total_size;

		asm_label(d->spel);
		asm_push(ASM_REG_BP);
		asm_output_new(asm_out_type_mov,
				asm_operand_new_reg(NULL, ASM_REG_BP),
				asm_operand_new_reg(NULL, ASM_REG_SP));

		curfunc_lblfin = asm_label_code(decl_spel(d));

		if(offset)
			gen_func_stack(d, offset);

		gen_stmt(d->func_code);

		asm_label(curfunc_lblfin);
		if(offset)
			asm_output_new(asm_out_type_add,
					asm_operand_new_reg(NULL, ASM_REG_SP),
					asm_operand_new_val(offset));

		asm_output_new(asm_out_type_leave, NULL, NULL);
		asm_output_new(asm_out_type_ret,   NULL, NULL);

		free(curfunc_lblfin);

	}else if(d->arrayinit){
		asm_declare_array(SECTION_DATA, d->arrayinit->label, d->arrayinit);

	}else if(d->init && !const_expr_is_zero(d->init)){
		asm_declare_single(cc_out[SECTION_DATA], d);

	}else{
		/* always resb, since we use decl_size() */
		asm_out_section(SECTION_BSS, "%s resb %d", d->spel, decl_size(d));
	}
}

void gen_asm(symtable *globs)
{
	decl **diter;
	for(diter = globs->decls; diter && *diter; diter++){
		decl *d = *diter;

		if(d->ignore)
			continue;

		if(!type_store_static_or_extern(d->type->store))
			asm_out_section(SECTION_TEXT, "global %s", d->spel);

		gen_asm_global(d);
	}

	//asm_out_flush();
}
