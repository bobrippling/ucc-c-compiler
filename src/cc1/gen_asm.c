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
#include "struct.h"

char *curfunc_lblfin; /* extern */

void gen_expr(expr *e, symtable *stab)
{
	e->f_gen(e, stab);
}

void gen_stmt(stmt *t)
{
	t->f_gen(t);
}

void gen_asm_global(decl *d)
{
	if(d->type->spec & spec_extern){
		/* should be fine... */
		asm_out_str(cc_out[SECTION_BSS], "extern %s", d->spel);
		return;
	}

	if(d->func_code){
		int offset;

		asm_label(d->spel);
		asm_push(ASM_REG_BP);
		asm_output_new(asm_out_type_mov,
				asm_operand_new_reg(NULL, ASM_REG_BP),
				asm_operand_new_reg(NULL, ASM_REG_SP));

		curfunc_lblfin = asm_label_code(d->spel);

		if((offset = d->func_code->symtab->auto_total_size)){
			asm_output_new(asm_out_type_sub,
					asm_operand_new_reg(NULL, ASM_REG_SP),
					asm_operand_new_val(NULL, offset));
		}

		gen_stmt(d->func_code);

		asm_label(curfunc_lblfin);
		if(offset)
			asm_output_new(asm_out_type_add,
					asm_operand_new_reg(NULL, ASM_REG_SP),
					asm_operand_new_val(NULL, offset));

		asm_out_str(cc_out[SECTION_TEXT], "leave");
		asm_out_str(cc_out[SECTION_TEXT], "ret");
		free(curfunc_lblfin);

	}else if(d->arrayinit){
		asm_declare_array(SECTION_DATA, d->arrayinit->label, d->arrayinit);

	}else if(d->init && !const_expr_is_zero(d->init)){
		asm_declare_single(cc_out[SECTION_DATA], d);

	}else{
		asm_out_str(cc_out[SECTION_BSS], "%s res%c %d", d->spel, asm_type_ch(d), decl_size(d));
	}
}

void gen_asm(symtable *globs)
{
	decl **diter;
	for(diter = globs->decls; diter && *diter; diter++){
		decl *d = *diter;

		if(d->ignore)
			continue;

		if(!(d->type->spec & spec_static) && !(d->type->spec & spec_extern))
			asm_out_str(cc_out[SECTION_TEXT], "global %s", d->spel);

		gen_asm_global(d);
	}
}
