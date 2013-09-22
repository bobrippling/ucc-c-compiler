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
#include "../out/basic_block.h"

#include "../pack.h"
#include "../sue.h"
#include "../funcargs.h"

#include "__builtin_va.h"

#include "../tokenise.h"
#include "../tokconv.h"
#include "../parse.h"
#include "../parse_type.h"
#include "__builtin_va.h"
#include "../type_ref_is.h"

static void va_type_check(expr *va_l, expr *in)
{
	/* we need to check decayed, since we may have
	 * f(va_list l)
	 * aka
	 * f(__builtin_va_list *l) [the array has decayed]
	 */
	if(!curdecl_func)
		die_at(&in->where, "%s() outside a function",
				BUILTIN_SPEL(in));

	if(type_ref_cmp(va_l->tree_type, type_ref_cached_VA_LIST_decayed(), 0)
				!= TYPE_EQUAL)
	{
		die_at(&va_l->where,
				"first argument to %s should be a va_list (not %s)",
				BUILTIN_SPEL(in), type_ref_to_str(va_l->tree_type));
	}
}

static void va_ensure_variadic(expr *e)
{
	funcargs *args = type_ref_funcargs(curdecl_func->ref);

	if(!args->variadic)
		die_at(&e->where, "%s in non-variadic function", BUILTIN_SPEL(e->expr));
}

static void fold_va_start(expr *e, symtable *stab)
{
	expr *va_l;

	if(dynarray_count(e->funcargs) != 2)
		die_at(&e->where, "%s requires two arguments", BUILTIN_SPEL(e->expr));

	va_l = e->funcargs[0];
	fold_inc_writes_if_sym(va_l, stab);

	FOLD_EXPR(e->funcargs[0], stab);
	FOLD_EXPR(e->funcargs[1], stab);

	va_l = e->funcargs[0];
	va_type_check(va_l, e->expr);

	va_ensure_variadic(e);

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
		struct
		{
			unsigned gp, fp;
		} nargs = { 0, 0 };
		funcargs *const fa = type_ref_funcargs(curdecl_func->ref);

		funcargs_ty_calc(fa, &nargs.gp, &nargs.fp);

		/* need to set the offsets to act as if we've skipped over
		 * n call regs, since we may already have some arguments used
		 */
		ADD_ASSIGN_VAL("gp_offset", nargs.gp * ws);
		ADD_ASSIGN_VAL("fp_offset", (6 + nargs.fp) * ws);
		/* FIXME: x86_64::N_CALL_REGS_I reference above */

		/* adjust to take the skip into account */
		ADD_ASSIGN("reg_save_area",
				expr_new_op2(op_minus,
					builtin_new_reg_save_area(), /* void arith - need _pws */
					expr_new_val((nargs.gp + nargs.fp) * ws))); /* total arg count * ws */

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

static basic_blk *builtin_gen_va_start(expr *e, basic_blk *b_from)
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
	out_push_zero(b_from, type_ref_new_INTPTR_T());
	out_store(b_from);
#else
	out_comment(b_from, "va_start() begin");
	gen_stmt(e->bits.variadic_setup, b_from);
	out_push_noop(b_from);
	out_comment(b_from, "va_start() end");
#endif

	return b_from;
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

static void va_arg_gen_read(
		expr *const e,
		type_ref *const ty,
		decl *const offset_decl, /* varies - float or integral */
		decl *const mem_reg_save_area,
		decl *const mem_overflow_arg_area,
		basic_blk *b_from)
{
	struct basic_blk *b_reg, *b_stk, *b_after;
	struct basic_blk_phi *b_join;

	/* FIXME: this needs to reference x86_64::N_CALL_REGS_{I,F} */
	const int fp = type_ref_is_floating(ty);
	const unsigned max_reg_args_sz = 6 * 8 + (fp ? 16 * 16 : 0);
	const unsigned ws = platform_word_size();
	const unsigned increment = fp ? 2 * ws : ws;

	gen_expr(e->lhs, b_from); /* va_list */
	out_change_type(b_from, type_ref_cached_VOID_PTR());
	out_dup(b_from); /* va, va */

	out_push_l(b_from, type_ref_cached_LONG(), offset_decl->struct_offset);
	out_op(b_from, op_plus); /* va, &va.gp_offset */

	/*out_set_lvalue(); * val.gp_offset is an lvalue */

	out_change_type(b_from, type_ref_cached_INT_PTR());
	out_dup(b_from); /* va, &gp_o, &gp_o */

	out_deref(b_from); /* va, &gp_o, gp_o */
	out_push_l(b_from, type_ref_cached_INT(), max_reg_args_sz);
	out_op(b_from, op_lt); /* va, &gp_o, <cond> */

	bb_split_new(b_from, &b_reg, &b_stk);

	/* register code */
	out_dup(b_reg); /* va, &gp_o, &gp_o */
	out_deref(b_reg); /* va, &gp_o, gp_o */

	/* increment either 8 for an integral, or 16 for a float argument
	 * since xmm0 are 128-bit registers, aka 16 byte
	 */
	out_push_l(b_reg, type_ref_cached_INT(), increment); /* pws */
	out_op(b_reg, op_plus); /* va, &gp_o, gp_o+ws */

	out_store(b_reg); /* va, gp_o+ws */
	out_push_l(b_reg, type_ref_cached_INT(), increment); /* pws */
	out_op(b_reg, op_minus); /* va, gp_o */
	out_change_type(b_reg, type_ref_cached_LONG());

	out_swap(b_reg); /* gp_o, va */
	out_push_l(b_reg, type_ref_cached_LONG(), mem_reg_save_area->struct_offset);
	out_op(b_reg, op_plus); /* gp_o, &reg_save_area */
	out_change_type(b_reg, type_ref_cached_LONG_PTR());
	out_deref(b_reg);
	out_swap(b_reg);
	out_op(b_reg, op_plus); /* reg_save_area + gp_o */

	/* --- stack code */
	gen_expr(e->lhs, b_stk);
	/* va */
	out_change_type(b_stk, type_ref_cached_VOID_PTR());
	out_push_l(b_stk, type_ref_cached_LONG(), mem_overflow_arg_area->struct_offset);
	out_op(b_stk, op_plus);
	/* &overflow_a */

	/*out_set_lvalue(); * overflow entry in the struct is an lvalue */

	out_dup(b_stk),
		out_change_type(b_stk, type_ref_cached_LONG_PTR()),
		out_deref(b_stk);
	/* &overflow_a, overflow_a */

	/* XXX: pws will need changing if we jump directly to stack, e.g. passing a struct */
	out_push_l(b_stk, type_ref_cached_LONG(), ws);
	out_op(b_stk, op_plus);

	out_store(b_stk);

	out_push_l(b_stk, type_ref_cached_LONG(), ws);
	out_op(b_stk, op_minus);

	/* ensure we match the other block's final result before the merge */
	b_join = bb_new_phi();
	bb_phi_incoming(b_join, b_reg);
	bb_phi_incoming(b_join, b_stk);
	b_after = bb_phi_next(b_join);

	/* now have a pointer to the right memory address */
	{
		type_ref *r_tmp = type_ref_new_ptr(ty, qual_none);
		out_change_type(b_after, r_tmp);
		out_deref(b_after);
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
}

static basic_blk *builtin_gen_va_arg(expr *e, basic_blk *b_from)
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
	char *lbl_else = out_label_code(b_from, "va_arg_overflow"),
			 *lbl_fin  = out_label_code(b_from, "va_arg_fin");

	out_comment(b_from, "va_arg start");

	lea_expr(e->lhs, stab);
	/* &va */

	out_dup(b_from);
	/* &va, &va */

	out_change_type(b_from, type_ref_new_LONG_PTR());
	out_deref(b_from);
	/* &va, va */

	/* out_n_call_regs(b_from) has been revoked - UCC ABI is obsolete */
	out_push_l(b_from, type_ref_new_LONG(), out_n_call_regs() - nargs);
	out_op(b_from, op_lt);
	/* &va, (<) */

	out_jfalse(b_from, lbl_else);
	/* &va */

	/* __builtin_frame_address(0) - nargs
	 * - multiply by pws is implicit - void *
	 */
	out_push_frame_ptr(b_from, 0);
	out_change_type(b_from, type_ref_new_LONG_PTR());
	out_push_l(b_from, type_ref_new_INTPTR_T(), nargs);
	out_op(b_from, op_minus);
	/* &va, va_ptr */

	/* - (intptr_t)val[0]++  */
	out_swap(b_from); /* pull &val to the top */

	/* va_ptr, &va */
	out_dup(b_from);
	out_change_type(b_from, type_ref_new_LONG_PTR());
	/* va_ptr, (long *)&va, (int *)&va */

	out_deref(b_from);
	/* va_ptr, &va, va */

	out_push_l(b_from, type_ref_new_INTPTR_T(), 1);
	out_op(b_from, op_plus); /* val[0]++ */
	/* va_ptr, &va, (va+1) */
	out_store(b_from);
	/* va_ptr, (va+1) */

	out_op(b_from, op_minus);
	/* va_ptr - (va+1) */
	/* va_ptr - va - 1 = va_ptr_arg-1 */

	EOF_WHERE(&e->where,
		out_change_type(b_from, type_ref_new_ptr(e->tree_type, qual_none));
	);
	out_deref(b_from);
	/* *va_arg() */

	out_push_lbl(b_from, lbl_fin, 0);
	out_jmp(b_from);
	out_label(b_from, lbl_else);

	out_comment(b_from, "TODO");
	out_undefined(b_from);

	out_label(b_from, lbl_fin);

	free(lbl_else);
	free(lbl_fin);

	out_comment(b_from, "va_arg end");
#elif defined(UCC_ABI_EXTERNAL)

	out_push_lbl(b_from, "__va_arg", 1);

	/* generate a call to abi.c's __va_arg */
	out_push_l(b_from, type_ref_new_LONG(), type_ref_size(e->bits.tref, NULL));
	/* 0 - abi.c's gen_reg. this is temporary until we have builtin_va_arg proper */
	out_push_zero(b_from, type_ref_new_INT());
	gen_expr(e->lhs);

	extern void *funcargs_new(); /* XXX: temporary hack for the call */

	out_call(b_from, 3, type_ref_new_ptr(e->bits.tref, qual_none),
			type_ref_new_func(type_ref_new_VOID(), funcargs_new()));

	out_deref(b_from); /* __va_arg returns a pointer to the stack location of the argument */
#else
	{
		type_ref *const ty = e->bits.tref;

		if(type_ref_is_s_or_u(ty)){
			ICE("TODO: s/u/e va_arg");
stack:
			ICE("TODO: stack __builtin_va_arg()");

		}else{
			const type *typ = type_ref_get_type(ty);
			const int fp = typ && type_floating(typ->primitive);
			struct_union_enum_st *sue_va;

			if(typ && typ->primitive == type_ldouble)
				goto stack;

			/* register */
			sue_va = type_ref_next(
					type_ref_cached_VA_LIST())->bits.type->sue;

#define VA_DECL(nam) \
			decl *mem_ ## nam = struct_union_member_find(sue_va, #nam, NULL, NULL)
			VA_DECL(gp_offset);
			VA_DECL(fp_offset);
			VA_DECL(reg_save_area);
			VA_DECL(overflow_arg_area);

			va_arg_gen_read(
					e,
					ty,
					fp ? mem_fp_offset : mem_gp_offset,
					mem_reg_save_area,
					mem_overflow_arg_area,
					b_from);
		}
	}

#endif

	return b_from;
}

static void fold_va_arg(expr *e, symtable *stab)
{
	type_ref *const ty = e->bits.tref;
	type_ref *to;

	FOLD_EXPR(e->lhs, stab);
	fold_type_ref(ty, NULL, stab);

	va_type_check(e->lhs, e->expr);

	if(type_ref_is_promotable(ty, &to)){
		char tbuf[TYPE_REF_STATIC_BUFSIZ];

		warn_at(&e->where,
				"va_arg(..., %s) has undefined behaviour - promote to %s",
				type_ref_to_str(ty), type_ref_to_str_r(tbuf, to));
	}

	e->tree_type = ty;

#ifdef UCC_VA_ABI
	/* finally store the number of arguments to this function */
	e->bits.n = dynarray_count(
			type_ref_funcargs(
				curdecl_func->ref)->arglist)
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

static basic_blk *builtin_gen_va_end(expr *e, basic_blk *b_from)
{
	(void)e;
	out_push_noop(b_from);

	return b_from;
}

static void fold_va_end(expr *e, symtable *stab)
{
	if(dynarray_count(e->funcargs) != 1)
		die_at(&e->where, "%s requires one argument", BUILTIN_SPEL(e->expr));

	FOLD_EXPR(e->funcargs[0], stab);
	va_type_check(e->funcargs[0], e->expr);

	va_ensure_variadic(e);

	e->tree_type = type_ref_cached_VOID();
}

expr *parse_va_end(void)
{
	expr *fcall = parse_any_args();
	expr_mutate_builtin_gen(fcall, va_end);
	return fcall;
}
