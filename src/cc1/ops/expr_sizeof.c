#include "ops.h"
#include "../sue.h"

#define SIZEOF_WHAT(e) ((e)->expr ? (e)->expr->tree_type : (e)->decl)
#define SIZEOF_SIZE(e)  (e)->bits.iv.val

const char *str_expr_sizeof()
{
	return "sizeof";
}

void fold_expr_sizeof(expr *e, symtable *stab)
{
	decl *chosen;

	if(e->expr)
		fold_expr(e->expr, stab);

	chosen = SIZEOF_WHAT(e);

	if(!e->expr_is_typeof){
		if(decl_has_incomplete_array(chosen))
			DIE_AT(&e->where, "sizeof incomplete array");

		if(decl_is_struct_or_union(chosen) && sue_incomplete(chosen->type->sue))
			DIE_AT(&e->where, "sizeof %s", type_to_str(chosen->type));
	}

	SIZEOF_SIZE(e) = decl_size(SIZEOF_WHAT(e));

	e->tree_type = decl_new();
	/* size_t */
	e->tree_type->type->primitive = type_int;
	e->tree_type->type->is_signed = 0;
}

void const_expr_sizeof(expr *e, intval *piv, enum constyness *pconst_type)
{
	UCC_ASSERT(e->tree_type, "const_fold on sizeof before fold");
	/* memcpy? */
	piv->val = SIZEOF_SIZE(e);
	*pconst_type = CONST_WITH_VAL;
}

void gen_expr_sizeof_1(expr *e)
{
	ICE("TODO: init with %s", e->f_str());
}

void gen_expr_sizeof(expr *e, symtable *stab)
{
	decl *d = SIZEOF_WHAT(e);
	(void)stab;

	asm_temp(1, "push %ld ; sizeof %s%s",
			SIZEOF_SIZE(e),
			e->expr ? "" : "type ",
			decl_to_str(d));
}

void gen_expr_str_sizeof(expr *e, symtable *stab)
{
	(void)stab;
	if(e->expr){
		idt_printf("sizeof expr:\n");
		print_expr(e->expr);
	}else{
		idt_printf("sizeof %s\n", decl_to_str(e->decl));
	}
	idt_printf("size = %ld\n", SIZEOF_SIZE(e));
}

void mutate_expr_sizeof(expr *e)
{
	e->f_const_fold = const_expr_sizeof;
}

expr *expr_new_sizeof_decl(decl *d, int is_typeof)
{
	expr *e = expr_new_wrapper(sizeof);
	e->decl = d;
	e->expr_is_typeof = is_typeof;
	return e;
}

expr *expr_new_sizeof_expr(expr *sizeof_this, int is_typeof)
{
	expr *e = expr_new_wrapper(sizeof);
	e->expr = sizeof_this;
	e->expr_is_typeof = is_typeof;
	return e;
}

void gen_expr_style_sizeof(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
