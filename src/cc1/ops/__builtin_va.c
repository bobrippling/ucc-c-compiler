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
#include "../pack.h"
#include "../sue.h"
#include "../funcargs.h"

#include "__builtin_va.h"

#include "../tokenise.h"
#include "../tokconv.h"
#include "../parse.h"
#include "../parse_type.h"

static unsigned current_func_args_cnt(symtable *stab)
{
	return dynarray_count(
			(decl **)type_ref_funcargs(
				symtab_func(stab)->ref)->arglist);
}

static void va_type_check(expr *va_l, expr *in, symtable *stab)
{
	/* we need to check decayed, since we may have
	 * f(va_list l)
	 * aka
	 * f(__builtin_va_list *l) [the array has decayed]
	 */
	if(!symtab_func(stab))
		die_at(&in->where, "%s() outside a function",
				BUILTIN_SPEL(in));

	if(!type_ref_equal(va_l->tree_type,
				type_ref_cached_VA_LIST_decayed(),
				DECL_CMP_EXACT_MATCH))
	{
		die_at(&va_l->where,
				"first argument to %s should be a va_list (not %s)",
				BUILTIN_SPEL(in), type_ref_to_str(va_l->tree_type));
	}
}

static void va_ensure_variadic(expr *e, symtable *stab)
{
	funcargs *args = type_ref_funcargs(symtab_func(stab)->ref);

	if(!args->variadic)
		die_at(&e->where, "%s in non-variadic function", BUILTIN_SPEL(e->expr));
}

static void fold_va_start(expr *e, symtable *stab)
{
	int n_args;
	expr *va_l;

	if(dynarray_count(e->funcargs) != 2)
		die_at(&e->where, "%s requires two arguments", BUILTIN_SPEL(e->expr));

	va_l = e->funcargs[0];
	fold_inc_writes_if_sym(va_l, stab);

	FOLD_EXPR(e->funcargs[0], stab);
	FOLD_EXPR(e->funcargs[1], stab);

	va_l = e->funcargs[0];
	va_type_check(va_l, e->expr, stab);

	va_ensure_variadic(e, stab);

	/* second arg check */
	{
		sym *second = NULL;
		decl **args = symtab_func_root(stab)->decls;
		sym *arg = args[dynarray_count(args) - 1]->sym;

		if(expr_kind(e->funcargs[1], identifier))
			second = e->funcargs[1]->bits.ident.sym;


		if(second != arg)
			warn_at(&e->funcargs[1]->where,
					"second parameter to va_start "
					"isn't last named argument");
	}

	n_args = current_func_args_cnt(stab);

#ifndef UCC_VA_ABI
	{
		stmt *assigns = stmt_new_wrapper(code, symtab_new(stab));
		expr *assign;

#define ADD_ASSIGN(memb, exp)               \
		assign = expr_new_assign(               \
				expr_new_struct(                    \
					va_l, 0 /* ->  since it's [1] */, \
					expr_new_identifier(memb)),       \
				exp);                               \
		                                        \
		dynarray_add(&assigns->codes,           \
				expr_to_stmt(assign, stab))

#define ADD_ASSIGN_VAL(memb, val) ADD_ASSIGN(memb, expr_new_val(val))

		const int ws = platform_word_size();
		const int n_args_pws = n_args * ws;

		/* need to set the offsets to act as if we've skipped over
		 * n call regs, since we may already have some arguments used
		 */
		ADD_ASSIGN_VAL("gp_offset", n_args_pws);
		ADD_ASSIGN_VAL("fp_offset", 0); /* TODO: when we have float support */

		/* adjust to take the skip into account */
		ADD_ASSIGN("reg_save_area",
				expr_new_op2(op_minus,
					builtin_new_reg_save_area(), /* void arith - need _pws */
					expr_new_val(n_args_pws)));

		ADD_ASSIGN("overflow_arg_area",
				expr_new_op2(op_plus,
					builtin_new_frame_address(0), /* *2 to step over saved-rbp and saved-ret */
					expr_new_val(ws * 2)));


		fold_stmt(assigns);
		e->bits.variadic_setup = assigns;
	}
#endif

	e->tree_type = type_ref_cached_VOID();
}

static void builtin_gen_va_start(expr *e)
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
	out_comment("va_start() begin");
	gen_stmt(e->bits.variadic_setup);
	out_push_noop();
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

static void builtin_gen_va_arg(expr *e)
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
#elif defined(UCC_ABI_EXTERNAL)

	out_push_lbl("__va_arg", 1);

	/* generate a call to abi.c's __va_arg */
	out_push_i(type_ref_new_LONG(), type_ref_size(e->bits.tref, NULL));
	/* 0 - abi.c's gen_reg. this is temporary until we have builtin_va_arg proper */
	out_push_i(type_ref_new_INT(), 0);
	gen_expr(e->lhs);

	extern void *funcargs_new(); /* XXX: temporary hack for the call */

	out_call(3, type_ref_new_ptr(e->bits.tref, qual_none),
			type_ref_new_func(type_ref_new_VOID(), funcargs_new()));

	out_deref(); /* __va_arg returns a pointer to the stack location of the argument */
#else
	{
		type_ref *const ty = e->bits.tref;

		if(type_ref_is_floating(ty)){
			const type *typ = type_ref_get_type(ty);

			if(typ->primitive == type_ldouble)
				goto stack;

			ICW("TODO: floating point");

		}else if(type_ref_is_s_or_u(ty)){
			ICE("TODO: s/u/e va_arg");
stack:
			ICE("TODO: stack __builtin_va_arg()");

		}else{
			/* register */
			char *lbl_stack = out_label_code("va_else");
			char *lbl_fin   = out_label_code("va_fin");
			char vphi_buf[OUT_VPHI_SZ];

			struct_union_enum_st *sue_va = type_ref_next(type_ref_cached_VA_LIST())->bits.type->sue;

#define VA_DECL(nam) \
			decl *mem_ ## nam = struct_union_member_find(sue_va, #nam, NULL, NULL)
			VA_DECL(gp_offset);
			VA_DECL(reg_save_area);
			VA_DECL(overflow_arg_area);

			gen_expr(e->lhs); /* va_list */
			out_change_type(type_ref_cached_VOID_PTR());
			out_dup(); /* va, va */

			out_push_i(type_ref_cached_LONG(), mem_gp_offset->struct_offset);
			out_op(op_plus); /* va, &va.gp_offset */

			out_change_type(type_ref_cached_INT_PTR());
			out_dup(); /* va, &gp_o, &gp_o */

			out_deref(); /* va, &gp_o, gp_o */
			out_push_i(type_ref_cached_INT(), 6 * 8); /* N_CALL_REGS * pws */
			out_op(op_lt); /* va, &gp_o, <cond> */
			out_jfalse(lbl_stack);

			/* register code */
			out_dup(); /* va, &gp_o, &gp_o */
			out_deref(); /* va, &gp_o, gp_o */

			out_push_i(type_ref_cached_INT(), 8); /* pws */
			out_op(op_plus); /* va, &gp_o, gp_o+8 */

			out_store(); /* va, gp_o+8 */
			out_push_i(type_ref_cached_INT(), 8); /* pws */
			out_op(op_minus); /* va, gp_o */
			out_change_type(type_ref_cached_LONG());

			out_swap(); /* gp_o, va */
			out_push_i(type_ref_cached_LONG(), mem_reg_save_area->struct_offset);
			out_op(op_plus); /* gp_o, &reg_save_area */
			out_change_type(type_ref_cached_LONG_PTR());
			out_deref();
			out_swap();
			out_op(op_plus); /* reg_save_area + gp_o */

			out_push_lbl(lbl_fin, 0);
			out_jmp();

			/* stack code */
			out_label(lbl_stack);

			/* prepare for joining later */
			out_phi_pop_to(&vphi_buf);

			gen_expr(e->lhs);
			/* va */
			out_change_type(type_ref_cached_VOID_PTR());
			out_push_i(type_ref_cached_LONG(), mem_overflow_arg_area->struct_offset);
			out_op(op_plus);
			/* &overflow_a */

			out_dup(), out_change_type(type_ref_cached_LONG_PTR()), out_deref();
			/* &overflow_a, overflow_a */

			/* XXX: 8 = pws, but will need changing if we jump directly to stack, e.g. passing a struct */
			out_push_i(type_ref_cached_LONG(), 8);
			out_op(op_plus);

			out_store();

			out_push_i(type_ref_cached_LONG(), 8);
			out_op(op_minus);

			/* ensure we match the other block's final result before the merge */
			out_phi_join(vphi_buf);

			/* "merge" */
			out_label(lbl_fin);

			/* now have a pointer to the right memory address */
			{
				type_ref *r_tmp = type_ref_new_ptr(ty, qual_none);
				out_change_type(r_tmp);
				out_deref();
				type_ref_free_1(r_tmp);
			}

			/*
			 * this works by using phi magic - we end up with something like this:
			 *
			 *   <reg calc>
			 *   // pointer in rbx
			 *   jmp fin
			 * else:
			 *   <stack calc>
			 *   // pointer in rax
			 *   <phi-merge with previous block>
			 * fin:
			 *   ...
			 *
			 * This is because the two parts of the if above are disjoint, one may
			 * leave its result in eax, one in ebx. We need basic blocks and phi:s to
			 * solve this properly.
			 *
			 * This problem exists in other code, such as &&-gen, but since we pop
			 * and push immediately, it doesn't manifest itself.
			 */

			free(lbl_stack);
			free(lbl_fin);
		}
	}

#endif
}

static void fold_va_arg(expr *e, symtable *stab)
{
	type_ref *const ty = e->bits.tref;

	FOLD_EXPR(e->lhs, stab);
	fold_type_ref(ty, NULL, stab);

	va_type_check(e->lhs, e->expr, stab);

	if(type_ref_size(ty, &e->lhs->where) < type_primitive_size(type_int)){
		warn_at(&e->where,
				"va_arg(..., %s) has undefined behaviour - promote to int",
				type_ref_to_str(ty));
	}

	e->tree_type = ty;

#ifdef UCC_VA_ABI
	/* finally store the number of arguments to this function */
	e->bits.n = current_func_args_cnt();
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

static void builtin_gen_va_end(expr *e)
{
	(void)e;
	out_push_noop();
}

static void fold_va_end(expr *e, symtable *stab)
{
	if(dynarray_count(e->funcargs) != 1)
		die_at(&e->where, "%s requires one argument", BUILTIN_SPEL(e->expr));

	FOLD_EXPR(e->funcargs[0], stab);
	va_type_check(e->funcargs[0], e->expr, stab);

	va_ensure_variadic(e, stab);

	e->tree_type = type_ref_cached_VOID();
}

expr *parse_va_end(void)
{
	expr *fcall = parse_any_args();
	expr_mutate_builtin_gen(fcall, va_end);
	return fcall;
}
