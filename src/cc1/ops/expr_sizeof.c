#include "ops.h"
#include "expr_sizeof.h"
#include "../sue.h"
#include "../out/asm.h"

#define SIZEOF_WHAT(e) ((e)->expr ? (e)->expr->tree_type : (e)->bits.sizeof_this)
#define SIZEOF_SIZE(e)  (e)->bits.iv.val

#define sizeof_this tref

const char *str_expr_sizeof()
{
	return "sizeof/typeof";
}

void fold_expr_sizeof(expr *e, symtable *stab)
{
	type_ref *chosen;

	if(e->expr)
		FOLD_EXPR_NO_DECAY(e->expr, stab);
	else
		fold_type_ref(e->bits.sizeof_this, NULL, stab);

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

	switch(e->what_of){
		case what_typeof:
			e->tree_type = chosen;
			break;

		case what_sizeof:
		{
			struct_union_enum_st *sue;

			if(!type_ref_is_complete(chosen))
				DIE_AT(&e->where, "sizeof incomplete type %s", type_ref_to_str(chosen));

			if((sue = type_ref_is_s_or_u(chosen)) && sue_incomplete(sue))
				DIE_AT(&e->where, "sizeof %s", type_ref_to_str(chosen));

			SIZEOF_SIZE(e) = type_ref_size(SIZEOF_WHAT(e), &e->where);

			{
				type *t;
				e->tree_type = type_ref_new_type(t = type_new_primitive(type_long));
				/* size_t */
				t->is_signed = 0;
			}
			break;
		}

		case what_alignof:
			ICE("TODO");
	}
}

void const_expr_sizeof(expr *e, consty *k)
{
	UCC_ASSERT(e->tree_type, "const_fold on sizeof before fold");
	memcpy_safe(&k->bits.iv, &e->bits.iv);
	k->type = CONST_VAL;
}

void gen_expr_sizeof(expr *e, symtable *stab)
{
	type_ref *r = SIZEOF_WHAT(e);
	(void)stab;

	out_push_i(e->tree_type, SIZEOF_SIZE(e));

	out_comment("sizeof %s%s", e->expr ? "" : "type ", type_ref_to_str(r));
}

void gen_expr_str_sizeof(expr *e, symtable *stab)
{
	(void)stab;
	if(e->expr){
		idt_printf("sizeof expr:\n");
		print_expr(e->expr);
	}else{
		idt_printf("sizeof %s\n", type_ref_to_str(e->bits.sizeof_this));
	}

	if(e->what_of == what_sizeof)
		idt_printf("size = %ld\n", SIZEOF_SIZE(e));
}

void mutate_expr_sizeof(expr *e)
{
	e->f_const_fold = const_expr_sizeof;
}

expr *expr_new_sizeof_type(type_ref *t, enum what_of what_of)
{
	expr *e = expr_new_wrapper(sizeof);
	e->bits.sizeof_this = t;
	e->what_of = what_of;
	return e;
}

expr *expr_new_sizeof_expr(expr *sizeof_this, enum what_of what_of)
{
	expr *e = expr_new_wrapper(sizeof);
	e->expr = sizeof_this;
	e->what_of = what_of;
	return e;
}

void gen_expr_style_sizeof(expr *e, symtable *stab)
{ (void)e; (void)stab; /* TODO */ }
