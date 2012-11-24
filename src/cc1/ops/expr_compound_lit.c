#include "ops.h"
#include "expr_compound_lit.h"
#include "../out/asm.h"
#include "../decl_init.h"

#define PARSED_DINIT(e) ((e)->val.init)

const char *str_expr_compound_lit(void)
{
	return "compound-lit";
}

void fold_expr_compound_lit(expr *e, symtable *stab)
{
	decl *d;

	if(e->code)
		return; /* being called from fold_gen_init_assignment_base */

	d = decl_new();
	d->ref = e->val.tref; /* from expr_cast */
	e->val.decl = d;

	/* must be set before the recursive fold_gen_init_assignment_base */
	e->tree_type = d->ref;

	e->sym = SYMTAB_ADD(stab, d, sym_local);

	if(stab->parent){
		/* non global - FIXME: check statics */
		e->code = stmt_new_wrapper(code, stab);

		decl_init_create_assignments_for_base(d, e, e->code);
		fold_stmt(e->code);
	}else{
		ICE("TODO: complete array");
		/*fold_complete_array(&d->ref, d->init);*/

		fold_decl_global_init(d, stab);
	}
}

static void gen_expr_compound_lit_code(expr *e)
{
	if(!e->expr_comp_lit_cgen){
		e->expr_comp_lit_cgen = 1;

		if(e->code->symtab->parent){
			gen_stmt(e->code);
		}else{
			/* global */
			ICE("TODO: global compound initialiser");
		}
	}
}

void gen_expr_compound_lit(expr *e, symtable *stab)
{
	(void)stab;

	/* allow (int){2}, but not (struct...){...} */
	fold_disallow_st_un(e, "compound literal");

	gen_expr_compound_lit_code(e);

	out_push_sym_val(e->sym);
}

static void lea_expr_compound_lit(expr *e, symtable *stab)
{
	(void)stab;

	gen_expr_compound_lit_code(e);

	out_push_sym(e->sym);
}

void const_expr_compound_lit(expr *e, intval *piv, enum constyness *pconst_type)
{
	(void)piv;

	*pconst_type = decl_init_is_const(PARSED_DINIT(e), NULL /* shouldn't be needed */)
		? CONST_WITHOUT_VAL : CONST_NO;
}

void gen_expr_str_compound_lit(expr *e, symtable *stab)
{
	decl *const d = e->val.decl;

	(void)stab;
	idt_printf("(%s){\n", decl_to_str(d));

	gen_str_indent++;
	print_decl(d,
			PDECL_NONE         |
			PDECL_INDENT       |
			PDECL_NEWLINE      |
			PDECL_SYM_OFFSET   |
			PDECL_FUNC_DESCEND |
			PDECL_PISDEF       |
			PDECL_PINIT        |
			PDECL_SIZE         |
			PDECL_ATTR);
	gen_str_indent--;

	idt_printf("}\n");
}

void gen_expr_style_compound_lit(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }

void mutate_expr_compound_lit(expr *e)
{
	e->f_lea = lea_expr_compound_lit;
	e->f_const_fold = const_expr_compound_lit;
}
