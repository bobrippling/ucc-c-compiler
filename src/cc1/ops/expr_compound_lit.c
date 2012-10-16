#include "ops.h"
#include "expr_compound_lit.h"

#define DINIT ((e)->val.init)

const char *str_expr_compound_lit(void)
{
	return "compound-lit";
}

void fold_expr_compound_lit(expr *e, symtable *stab)
{
	if(e->code)
		return; /* being called from fold_gen_init_assignment_base */

	e->code = stmt_new_wrapper(code, stab);

	fold_decl(e->decl, stab);
	e->decl->init = DINIT;

	/* must be set before the recursive fold_gen_init_assignment_base */
	e->tree_type = decl_copy_keep_array(e->decl);

	e->sym = SYMTAB_ADD(stab, e->decl, sym_local);

	fold_gen_init_assignment_base(e, e->decl, e->code);
	fold_stmt(e->code);
}

static void gen_expr_compound_lit_code(expr *e)
{
	if(!e->expr_comp_lit_cgen){
		e->expr_comp_lit_cgen = 1;
		gen_stmt(e->code);
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

void gen_expr_str_compound_lit(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("(%s){\n", decl_to_str(e->decl));

	gen_str_indent++;
	print_decl(e->decl,
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
}
