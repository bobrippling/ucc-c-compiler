#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "../../util/alloc.h"
#include "ops.h"

const char *str_expr_cast()
{
	return "cast";
}

int fold_const_expr_cast(expr *e)
{
	return const_fold(e->expr);
}

void fold_expr_cast(expr *e, symtable *stab)
{
	int size_lhs, size_rhs;
	decl *dlhs, *drhs;

	fold_expr(e->expr, stab);

	/*
	 * if we don't have a valid tree_type, get one
	 * this is only the case where we're involving a tdef or typeof
	 */

	if(e->tree_type->type->primitive == type_unknown){
		decl_free(e->tree_type);
		e->tree_type = decl_copy(e->expr->tree_type);
	}

#ifdef FLATTEN_CASTS
	if(expr_kind(e->expr, cast)){
		/* get rid of e->expr, replace with e->expr->rhs */
		expr *del = e->expr;

		e->expr = e->expr->expr;

		/*decl_free(del->tree_type); XXX: memleak */
		expr_free(del);

		fold_expr_cast(e, stab);
	}
#endif

	dlhs = e->tree_type;
	drhs = e->expr->tree_type;

	if((size_lhs = asm_type_size(dlhs)) < (size_rhs = asm_type_size(drhs))){
		char buf[DECL_STATIC_BUFSIZ];

		strcpy(buf, decl_to_str(drhs));

		warn_at(&e->where, "possible loss of precision %s, size %d <-- %s, size %d",
				decl_to_str(dlhs), size_lhs,
				buf, size_rhs);
	}
}

void gen_expr_cast_1(expr *e, FILE *f)
{
	e->expr->f_gen_1(e, f);
}

void gen_expr_cast(expr *e, symtable *stab)
{
	char buf[DECL_STATIC_BUFSIZ];
	decl *dlhs, *drhs;
	int size_lhs, size_rhs;

	gen_expr(e->expr, stab);

	dlhs = e->tree_type;
	drhs = e->expr->tree_type;

	/* type convert */
	strcpy(buf, decl_to_str(drhs));
	asm_comment("cast %s to %s", buf, decl_to_str(dlhs));

	/* check float <--> int conversion */
	if(decl_is_float(dlhs) != decl_is_float(drhs))
		ICE("TODO: float <-> int casting");

	/* decide if casting to a larger or smaller type */
	if((size_lhs = asm_type_size(dlhs)) != (size_rhs = asm_type_size(drhs))){
		asm_output *o;

		if(size_rhs > size_lhs){
			/* loss of precision, touch crabcakes */
			asm_comment("loss of precision, noop cast");
		}else{
			asm_pop(NULL, ASM_REG_A);
			/*
			* movsx -> mov sign extend
			* movsx rax, eax ; long <- int, etc
			* or
			* cbw  (ax  <- al)
			* cwde (eax <- ax)
			* cdqe (rax <- eax)
			*/
			/*o->extra = ustrprintf("c%c%s", ..) */

			/* movsx a..., a... */
			o = asm_output_new(asm_out_type_mov,
					asm_operand_new_reg(dlhs, ASM_REG_A),
					asm_operand_new_reg(drhs, ASM_REG_A));

			o->extra = ustrdup("sx");

			asm_push(ASM_REG_A);

			asm_comment("cast finish");
		}
	}else{
		asm_comment("cast - asm type sizes match (%d)", asm_type_size(dlhs));
	}
}

void gen_expr_str_cast(expr *e, symtable *stab)
{
	(void)stab;
	idt_printf("cast expr:\n");
	gen_str_indent++;
	print_expr(e->expr);
	gen_str_indent--;
}

expr *expr_new_cast(decl *to)
{
	expr *e = expr_new_wrapper(cast);
	e->tree_type = to;

	e->f_const_fold = fold_const_expr_cast;
	e->f_gen_1      = gen_expr_cast_1;
	return e;
}
