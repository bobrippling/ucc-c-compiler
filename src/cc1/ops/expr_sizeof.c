#include <string.h>

#include "ops.h"
#include "expr_sizeof.h"
#include "../sue.h"
#include "../out/asm.h"

#define SIZEOF_WHAT(e) ((e)->expr ? (e)->expr->tree_type : (e)->bits.size_of.of_type)
#define SIZEOF_SIZE(e)  (e)->bits.size_of.sz

#define sizeof_this tref

static const char *sizeof_what(enum what_of wo)
{
	switch(wo){
		case what_typeof:
			return "typeof";
		case what_sizeof:
			return "sizeof";
		case what_alignof:
			return "alignof";
	}

	return NULL;
}

const char *str_expr_sizeof()
{
	return "sizeof/typeof/alignof";
}

void fold_expr_sizeof(expr *e, symtable *stab)
{
	type_ref *chosen;

	if(e->expr)
		fold_expr_no_decay(e->expr, stab);
	else
		fold_type_ref(e->bits.size_of.of_type, NULL, stab);

	chosen = SIZEOF_WHAT(e);

	fold_check_expr(e->expr,
			FOLD_CHK_NO_BITFIELD,
			sizeof_what(e->what_of));

	switch(e->what_of){
		case what_typeof:
			e->tree_type = chosen;
			break;

		case what_sizeof:
		{
			/* check for sizeof array parameter */
			if(type_ref_is_decayed_array(chosen)){
				char ar_buf[TYPE_REF_STATIC_BUFSIZ];

				warn_at(&e->where, "array parameter size is sizeof(%s), not sizeof(%s)",
						type_ref_to_str(chosen),
						type_ref_to_str_r_show_decayed(ar_buf, chosen));
			}
		} /* fall */

		case what_alignof:
		{
			struct_union_enum_st *sue;
			int set = 0; /* need this, since .bits can't be relied upon to be 0 */

			if(!type_ref_is_complete(chosen))
				die_at(&e->where, "sizeof incomplete type %s", type_ref_to_str(chosen));

			if((sue = type_ref_is_s_or_u(chosen)) && !sue_complete(sue))
				die_at(&e->where, "sizeof %s", type_ref_to_str(chosen));

			if(e->what_of == what_alignof && e->expr){
				decl *d = NULL;

				if(expr_kind(e->expr, identifier))
					d = e->expr->bits.ident.sym->decl;
				else if(expr_kind(e->expr, struct))
					d = e->expr->bits.struct_mem.d;

				if(d)
					SIZEOF_SIZE(e) = decl_align(d), set = 1;
			}

			if(!set)
				SIZEOF_SIZE(e) = (e->what_of == what_sizeof
						? type_ref_size : type_ref_align)(
							SIZEOF_WHAT(e), &e->where);

			/* size_t */
			e->tree_type = type_ref_new_type(type_new_primitive(type_ulong));
			break;
		}
	}
}

static void const_expr_sizeof(expr *e, consty *k)
{
	UCC_ASSERT(e->tree_type, "const_fold on sizeof before fold");
	memset(k, 0, sizeof *k);
	k->bits.num.val.i = SIZEOF_SIZE(e);
	k->bits.num.suffix = VAL_UNSIGNED | VAL_LONG;
	k->type = CONST_NUM;
}

basic_blk *gen_expr_sizeof(expr *e, basic_blk *bb)
{
	type_ref *r = SIZEOF_WHAT(e);

	out_push_l(bb, e->tree_type, SIZEOF_SIZE(e));

	out_comment(bb, "sizeof %s%s", e->expr ? "" : "type ", type_ref_to_str(r));

	return bb;
}

basic_blk *gen_expr_str_sizeof(expr *e, basic_blk *bb)
{
	if(e->expr){
		idt_printf("sizeof expr:\n");
		print_expr(e->expr);
	}else{
		idt_printf("sizeof %s\n", type_ref_to_str(e->bits.size_of.of_type));
	}

	if(e->what_of == what_sizeof)
		idt_printf("size = %d\n", SIZEOF_SIZE(e));

	return bb;
}

void mutate_expr_sizeof(expr *e)
{
	e->f_const_fold = const_expr_sizeof;
}

expr *expr_new_sizeof_type(type_ref *t, enum what_of what_of)
{
	expr *e = expr_new_wrapper(sizeof);
	e->bits.size_of.of_type = t;
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

basic_blk *gen_expr_style_sizeof(expr *e, basic_blk *bb)
{
	stylef("%s(", sizeof_what(e->what_of));

	if(e->expr)
		bb = gen_expr(e->expr, bb);
	else
		stylef("%s", type_ref_to_str(e->bits.size_of.of_type));

	stylef(")");

	return bb;
}
