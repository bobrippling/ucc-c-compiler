#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../util/util.h"
#include "../util/alloc.h"

#include "cc1.h"
#include "type.h"
#include "decl.h"
#include "expr.h"
#include "type_is.h"
#include "const.h"

#include "cc1_where.h"

void expr_mutate(expr *e, func_mutate_expr *f,
		func_fold *f_fold,
		func_str *f_str,
		func_gen *f_gen,
		func_dump *f_dump,
		func_gen *f_gen_style
		)
{
	e->f_fold = f_fold;
	e->f_str  = f_str;
	e->f_dump = f_dump;

	switch(cc1_backend){
		case BACKEND_DUMP:
			e->f_gen = NULL;
			break;
		case BACKEND_ASM:
			e->f_gen = f_gen;
			break;
		case BACKEND_STYLE:
			e->f_gen = f_gen_style;
			break;

		default: ICE("bad backend");
	}

	e->f_const_fold = NULL;

	f(e);
}

expr *expr_new(func_mutate_expr *f,
		func_fold *f_fold,
		func_str *f_str,
		func_gen *f_gen,
		func_dump *f_dump,
		func_gen *f_gen_style)
{
	expr *e = umalloc(sizeof *e);
	where_cc1_current(&e->where);
	expr_mutate(e, f, f_fold, f_str, f_gen, f_dump, f_gen_style);
	return e;
}

const char *expr_str_friendly(expr *e)
{
	return expr_skip_generated_casts(e)->f_str();
}

expr *expr_set_where(expr *e, where const *w)
{
	memcpy_safe(&e->where, w);
	return e;
}

expr *expr_set_where_len(expr *e, where *start)
{
	where end;
	where_cc1_current(&end);

	expr_set_where(e, start);
	if(start->line == end.line)
		e->where.len = end.chr - start->chr;

	return e;
}

void expr_set_const(expr *e, consty *k)
{
	e->const_eval.const_folded = 1;
	memcpy_safe(&e->const_eval.k, k);
}

int expr_is_null_ptr(expr *e, enum null_strictness ty)
{
	/* 6.3.2.3:
	 *
	 * An integer constant expression with the value 0, or such an expression
	 * cast to type void *, is called a null pointer constant
	 *
	 * NULL_STRICT_ANY_PTR is used for sentinel checks,
	 * i.e. any null type pointer
	 */

	int b = 0;
	type *pointed_ty = type_is_ptr(e->tree_type);

	/* void * always qualifies */
	if(pointed_ty
	&& type_qual(pointed_ty) == qual_none
	&& type_is_primitive(pointed_ty, type_void))
	{
		b = 1;
	}else if(ty == NULL_STRICT_INT && type_is_integral(e->tree_type)){
		b = 1;
	}else if(ty == NULL_STRICT_ANY_PTR && type_is_ptr(e->tree_type)){
		b = 1;
	}

	return b && const_expr_and_zero(e);
}

enum lvalue_kind expr_is_lval(expr *e)
{
	if(e->f_islval)
		return e->f_islval(e);

	return LVALUE_NO;
}

enum lvalue_kind expr_is_lval_always(expr *e)
{
	(void)e;
	return LVALUE_USER_ASSIGNABLE;
}

enum lvalue_kind expr_is_lval_struct(expr *e)
{
	(void)e;
	return LVALUE_STRUCT;
}

expr *expr_new_array_idx_e(expr *base, expr *idx)
{
	expr *op = expr_new_op(op_plus);
	op->lhs = base;
	op->rhs = idx;
	return expr_new_deref(op);
}

expr *expr_new_array_idx(expr *base, int i)
{
	return expr_new_array_idx_e(base, expr_new_val(i));
}

expr *expr_skip_all_casts(expr *e)
{
	while(e && expr_kind(e, cast))
		e = e->expr;
	return e;
}

expr *expr_skip_lval2rval(expr *e)
{
	while(e && expr_kind(e, cast) && expr_cast_is_lval2rval(e))
		e = e->expr;
	return e;
}

expr *expr_skip_implicit_casts(expr *e)
{
	while(e && expr_kind(e, cast) && expr_cast_is_implicit(e))
		e = e->expr;
	return e;
}

expr *expr_skip_generated_casts(expr *e)
{
	while(e && expr_kind(e, cast) && (expr_cast_is_implicit(e) || expr_cast_is_lval2rval(e)))
		e = e->expr;
	return e;
}

decl *expr_to_declref(expr *e, const char **whynot)
{
	e = expr_skip_all_casts(e);

	if(expr_kind(e, identifier)){
		if(e->bits.ident.type == IDENT_NORM)
			return e->bits.ident.bits.ident.sym->decl;
		else if(whynot)
			*whynot = "not normal identifier";

	}else if(expr_kind(e, struct)){
		return e->bits.struct_mem.d;

	}else if(expr_kind(e, block)){
		return e->bits.block.sym->decl;

	}else if(whynot){
		*whynot = "not an identifier, member or block";
	}

	return NULL;
}

expr *expr_compiler_generated(expr *e)
{
	e->freestanding = 1;
	return e;
}
