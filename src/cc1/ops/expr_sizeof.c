#include "ops.h"
#include "expr_sizeof.h"
#include "../sue.h"
#include "../out/asm.h"

#define SIZEOF_WHAT(e) ((e)->expr ? (e)->expr->tree_type : (e)->val.sizeof_this)
#define SIZEOF_SIZE(e)  (e)->val.iv.val

const char *str_expr_sizeof()
{
	return "sizeof";
}

void fold_expr_sizeof(expr *e, symtable *stab)
{
	decl *chosen;

	if(e->expr)
		FOLD_EXPR(e->expr, stab);

	chosen = SIZEOF_WHAT(e);

#ifdef FIELD_WIDTH_TODO
	if(chosen->field_width){
		if(e->expr_is_typeof){
			WARN_AT(&e->where, "typeof applied to a bit-field - using underlying type");

			chosen->field_width = NULL;
			/* not a memleak
			 * should still be referenced from the decl we copied,
			 * or the struct type, etc
			 */
		}else{
			DIE_AT(&e->where, "sizeof applied to a bit-field");
		}
	}
#endif

	if(e->expr_is_typeof){
		e->tree_type = decl_copy(chosen);
	}else{
		struct_union_enum_st *sue;

		if(decl_is_incomplete_array(chosen))
			DIE_AT(&e->where, "sizeof incomplete array");

		if((sue = decl_is_struct_or_union(chosen)) && sue_incomplete(sue))
			DIE_AT(&e->where, "sizeof %s", decl_to_str(chosen));

		SIZEOF_SIZE(e) = decl_size(SIZEOF_WHAT(e));

		{
			type *t;
			e->tree_type = type_ref_new_type(t = type_new_primitive(type_long));
			/* size_t */
			t->is_signed = 0;
		}
	}
}

void const_expr_sizeof(expr *e, intval *piv, enum constyness *pconst_type)
{
	UCC_ASSERT(e->tree_type, "const_fold on sizeof before fold");
	/* memcpy? */
	piv->val = SIZEOF_SIZE(e);
	*pconst_type = CONST_WITH_VAL;
}

void static_expr_sizeof_store(expr *e)
{
	asm_declare_partial("%ld", SIZEOF_SIZE(e));
}

void gen_expr_sizeof(expr *e, symtable *stab)
{
	decl *d = SIZEOF_WHAT(e);
	(void)stab;

	out_push_i(e->tree_type, SIZEOF_SIZE(e));

	out_comment("sizeof %s%s", e->expr ? "" : "type ", decl_to_str(d));
}

void gen_expr_str_sizeof(expr *e, symtable *stab)
{
	(void)stab;
	if(e->expr){
		idt_printf("sizeof expr:\n");
		print_expr(e->expr);
	}else{
		idt_printf("sizeof %s\n", decl_to_str(e->val.sizeof_this));
	}

	if(!e->expr_is_typeof)
		idt_printf("size = %ld\n", SIZEOF_SIZE(e));
}

void mutate_expr_sizeof(expr *e)
{
	e->f_const_fold = const_expr_sizeof;
	e->f_static_addr = static_expr_sizeof_store;
}

expr *expr_new_sizeof_decl(decl *d, int is_typeof)
{
	expr *e = expr_new_wrapper(sizeof);
	e->val.sizeof_this = d;
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
