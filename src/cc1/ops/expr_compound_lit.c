#include "ops.h"
#include "expr_compound_lit.h"

#define dinit ((e)->val.init)

const char *str_expr_compound_lit(void)
{
	return "compound-lit";
}

void fold_expr_compound_lit(expr *e, symtable *stab)
{
	e->code = stmt_new_wrapper(code, stab);

	fold_decl(e->decl, stab);
	e->decl->init = dinit;

	fold_gen_init_assignment(e->decl, e->code);

	e->tree_type = decl_copy_keep_array(e->decl);
}

void gen_expr_compound_lit(expr *e, symtable *stab)
{
	(void)e;
	(void)stab;
	ICE("TODO: gen_expr_(cast){}");
}

void gen_expr_str_compound_lit(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("(%s){}:\n", decl_to_str(e->decl));
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
}

void gen_expr_style_compound_lit(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }

void mutate_expr_compound_lit(expr *e)
{
	(void)e;
}
