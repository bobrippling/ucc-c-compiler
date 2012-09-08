#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "../util/util.h"
#include "../util/dynarray.h"
#include "data_structs.h"
#include "cc1.h"
#include "macros.h"
#include "../util/platform.h"
#include "sym.h"
#include "gen_asm.h"
#include "../util/util.h"
#include "const.h"
#include "tree.h"
#include "out/out.h"
#include "out/lbl.h"
#include "out/asm.h"

char *curfunc_lblfin; /* extern */

void gen_expr(expr *e, symtable *stab)
{
	intval iv;
	enum constyness type;

	const_fold(e, &iv, &type);

	if(type == CONST_WITH_VAL) /* TODO: -O0 skips this */
		out_push_iv(e->tree_type, &iv);
	else
		e->f_gen(e, stab);
}

void lea_expr(expr *e, symtable *stab)
{
	UCC_ASSERT(e->f_store, "invalid store expression %s (no f_store())", e->f_str());

	e->f_store(e, stab);
}

void gen_stmt(stmt *t)
{
	t->f_gen(t);
	/* can't assert vtop != null here, since ({}) depend on this */
}

void static_store(expr *e)
{
	UCC_ASSERT(e->f_static_addr, "no static store for %s", e->f_str());
	e->f_static_addr(e);
}

#ifdef FANCY_STACK_INIT
#define ITER_DECLS(i) for(i = df->func_code->symtab->decls; i && *i; i++)

void gen_func_stack(decl *df, const int offset)
{
	int use_sub = 1, clever = 0;
	decl **iter;

	ITER_DECLS(iter)
		if((*iter)->init){
			clever = 1;
			break;
		}

	if(use_sub){
		asm_output_new(asm_out_type_sub,
				asm_operand_new_reg(NULL, ASM_REG_SP),
				asm_operand_new_val(offset));
	}else{
		ITER_DECLS(iter){
			decl *d = *iter;
			if(d->init && d->init->type != decl_init_scalar){
				ICW("TODO: stack gen or expr for %s init", decl_to_str(d));
			}
		}
	}
}
#else
#endif

void gen_asm_extern(decl *d)
{
	(void)d;
	/*asm_comment("extern %s", d->spel);*/
	/*asm_out_section(SECTION_BSS, "extern %s", d->spel);*/
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
		int nargs = 0;
		decl **aiter;

		for(aiter = d->func_code->symtab->decls; aiter && *aiter; aiter++)
			if((*aiter)->sym->type == sym_arg)
				nargs++;

		out_label(d->spel);

		out_func_prologue(
				d->func_code->symtab->auto_total_size,
				nargs, decl_funcargs(d)->variadic);

		curfunc_lblfin = out_label_code(d->spel);

		gen_stmt(d->func_code);

		out_label(curfunc_lblfin);

		out_func_epilogue();

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
				asm_out_section(SECTION_TEXT, ".globl %s\n", d->spel);
		}

		gen_asm_global(d);
	}

	out_assert_vtop_null();
}
