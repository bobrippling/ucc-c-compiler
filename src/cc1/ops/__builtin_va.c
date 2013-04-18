#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../data_structs.h"
#include "../expr.h"
#include "__builtin.h"
#include "__builtin_va.h"

#include "../cc1.h"
#include "../fold.h"
#include "../gen_asm.h"
#include "../out/out.h"
#include "../out/lbl.h"

#include "__builtin_va.h"

#include "../tokenise.h"
#include "../tokconv.h"
#include "../parse.h"
#include "../parse_type.h"

static void va_type_check(expr *va_l, expr *in)
{
	if(!type_ref_equal(va_l->tree_type,
				type_ref_new_VA_LIST(),
				DECL_CMP_EXACT_MATCH))
	{
		DIE_AT(&va_l->where,
				"first argument to %s should be a va_list",
				BUILTIN_SPEL(in));
	}
}

static void va_ensure_variadic(expr *e)
{
	funcargs *args = type_ref_funcargs(curdecl_func->ref);

	if(!args->variadic)
		DIE_AT(&e->where, "%s in non-variadic function", BUILTIN_SPEL(e->expr));
}

static void fold_va_start(expr *e, symtable *stab)
{
	expr *va_l;

	if(dynarray_count((void **)e->funcargs) != 2)
		DIE_AT(&e->where, "%s requires two arguments", BUILTIN_SPEL(e->expr));

	va_l = e->funcargs[0];
	fold_inc_writes_if_sym(va_l, stab);

	FOLD_EXPR_NO_DECAY(e->funcargs[0], stab);
	FOLD_EXPR(         e->funcargs[1], stab);

	va_l = e->funcargs[0];
	va_type_check(va_l, e->expr);

	va_ensure_variadic(e);
	/* TODO: check funcargs[1] is last argument to the function */

#ifndef UCC_VA_ABI
	{
		stmt *assigns = stmt_new_wrapper(code, symtab_new(stab));
		expr *assign;

#define ADD_ASSIGN(memb, exp)               \
		assign = expr_new_assign(               \
				expr_new_struct(                    \
					expr_new_deref(                   \
						va_l),                          \
					1 /* dot */,                      \
					expr_new_identifier(memb)),       \
				exp);                               \
		                                        \
		dynarray_add((void ***)&assigns->codes, \
				expr_to_stmt(assign, stab))

#define ADD_ASSIGN_VAL(memb, val) ADD_ASSIGN(memb, expr_new_val(val))

		/* zero the offsets */
		ADD_ASSIGN_VAL("gp_offset",         0);
		ADD_ASSIGN_VAL("fp_offset",         0);

		ADD_ASSIGN("reg_save_area",     builtin_new_reg_save_area());

		ADD_ASSIGN("overflow_arg_area",
				expr_new_op2(op_plus,
					builtin_new_frame_address(0),
					expr_new_val(platform_word_size())));


		fold_stmt(assigns);
		e->bits.variadic_setup = assigns;
	}
#endif

	e->tree_type = type_ref_new_VOID();
}

static void gen_va_start(expr *e, symtable *stab)
{
#ifdef UCC_VA_ABI
	/*
	 * va_list is 8-bytes. use the second 4 as the int
	 * offset into saved_reg_args. if this is >= N_CALL_REGS,
	 * we go into saved_var_args (see out/out.c::push_sym sym_arg
	 * for reference)
	 *
	 * assign to
	 *   e->funcargs[0]
	 * from
	 *   0L
	 */
	lea_expr(e->funcargs[0], stab);
	out_push_i(type_ref_new_INTPTR_T(), 0);
	out_store();
#else
	(void)stab;

	out_comment("va_start() begin");
	gen_stmt(e->bits.variadic_setup);
	out_push_i(type_ref_new_INT(), 0);
	out_comment("va_start() end");
#endif
}

expr *parse_va_start(void)
{
	/* va_start(__builtin_va_list &, identifier)
	 * second argument may be any expression - we don't use it
	 */
	expr *fcall = parse_any_args();
	expr_mutate_builtin_gen(fcall, va_start);
	return fcall;
}

static void gen_va_arg(expr *e, symtable *stab)
{
#ifdef UCC_VA_ABI
	/*
	 * first 4 bytes are offset into saved_regs
	 * second 4 bytes are offset into saved_var_stack
	 *
	 * va_list val;
	 *
	 * if(val[0] + nargs < N_CALL_REGS)
	 *   __builtin_frame_address(0) - pws * (nargs) - (intptr_t)++val[0]
	 * else
	 *   __builtin_frame_address(0) + pws() * (nargs - N_CALL_REGS) + val[1]++
	 *
	 * if becomes:
	 *   if(val[0] < N_CALL_REGS - nargs)
	 * since N_CALL_REGS-nargs can be calculated at compile time
	 */
	/* finally store the number of arguments to this function */
	const int nargs = e->bits.n;
	char *lbl_else = out_label_code("va_arg_overflow"),
			 *lbl_fin  = out_label_code("va_arg_fin");

	out_comment("va_arg start");

	lea_expr(e->lhs, stab);
	/* &va */

	out_dup();
	/* &va, &va */

	out_change_type(type_ref_new_LONG_PTR());
	out_deref();
	/* &va, va */

	out_push_i(type_ref_new_LONG(), out_n_call_regs() - nargs);
	out_op(op_lt);
	/* &va, (<) */

	out_jfalse(lbl_else);
	/* &va */

	/* __builtin_frame_address(0) - nargs
	 * - multiply by pws is implicit - void *
	 */
	out_push_frame_ptr(0);
	out_change_type(type_ref_new_LONG_PTR());
	out_push_i(type_ref_new_INTPTR_T(), nargs);
	out_op(op_minus);
	/* &va, va_ptr */

	/* - (intptr_t)val[0]++  */
	out_swap(); /* pull &val to the top */

	/* va_ptr, &va */
	out_dup();
	out_change_type(type_ref_new_LONG_PTR());
	/* va_ptr, (long *)&va, (int *)&va */

	out_deref();
	/* va_ptr, &va, va */

	out_push_i(type_ref_new_INTPTR_T(), 1);
	out_op(op_plus); /* val[0]++ */
	/* va_ptr, &va, (va+1) */
	out_store();
	/* va_ptr, (va+1) */

	out_op(op_minus);
	/* va_ptr - (va+1) */
	/* va_ptr - va - 1 = va_ptr_arg-1 */

	EOF_WHERE(&e->where,
		out_change_type(type_ref_new_ptr(e->tree_type, qual_none));
	);
	out_deref();
	/* *va_arg() */

	out_push_lbl(lbl_fin, 0);
	out_jmp();
	out_label(lbl_else);

	out_comment("TODO");
	out_undefined();

	out_label(lbl_fin);

	free(lbl_else);
	free(lbl_fin);

	out_comment("va_arg end");
#else

	out_push_lbl("__va_arg", 1);

	/* generate a call to abi.c's __va_arg */
	out_push_i(type_ref_new_LONG(), type_ref_size(e->bits.tref, NULL));
	/* 0 - abi.c's gen_reg. this is temporary until we have builtin_va_arg proper */
	out_push_i(type_ref_new_INT(), 0);
	gen_expr(e->lhs, stab);

	extern void *funcargs_new(); /* XXX: temporary hack for the call */

	out_call(3, type_ref_new_ptr(e->bits.tref, qual_none),
			type_ref_new_func(type_ref_new_VOID(), funcargs_new()));

	out_deref(); /* __va_arg returns a pointer to the stack location of the argument */
#endif
}

static void fold_va_arg(expr *e, symtable *stab)
{
	type_ref *const ty = e->bits.tref;

	FOLD_EXPR_NO_DECAY(e->lhs, stab);
	fold_type_ref(ty, NULL, stab);

	va_type_check(e->lhs, e->expr);

	if(type_ref_size(ty, &e->lhs->where) < type_primitive_size(type_int)){
		WARN_AT(&e->lhs->where,
				"va_arg(..., %s) has undefined behaviour - promote to int",
				type_ref_to_str(ty));
	}

	e->tree_type = ty;

#ifdef UCC_VA_ABI
	/* finally store the number of arguments to this function */
	e->bits.n = dynarray_count((void **)type_ref_funcargs(curdecl_func->ref)->arglist);
#endif
}

expr *parse_va_arg(void)
{
	/* va_arg(list, type) */
	expr *fcall = expr_new_funcall();
	expr *list = parse_expr_no_comma();
	type_ref *ty;

	EAT(token_comma);
	ty = parse_type();

	fcall->lhs = list;
	fcall->bits.tref = ty;

	expr_mutate_builtin_gen(fcall, va_arg);

	return fcall;
}

static void gen_va_end(expr *e, symtable *stab)
{
	(void)e;
	(void)stab;
	out_push_noop();
}

static void fold_va_end(expr *e, symtable *stab)
{
	if(dynarray_count((void **)e->funcargs) != 1)
		DIE_AT(&e->where, "%s requires one argument", BUILTIN_SPEL(e->expr));

	FOLD_EXPR_NO_DECAY(e->funcargs[0], stab);
	va_type_check(e->funcargs[0], e->expr);

	va_ensure_variadic(e);

	e->tree_type = type_ref_new_VOID();
}

expr *parse_va_end(void)
{
	expr *fcall = parse_any_args();
	expr_mutate_builtin_gen(fcall, va_end);
	return fcall;
}
