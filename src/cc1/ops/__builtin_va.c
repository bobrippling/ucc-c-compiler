#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/platform.h"
#include "../expr.h"
#include "__builtin.h"
#include "__builtin_va.h"

#include "../cc1.h"
#include "../fold.h"
#include "../gen_asm.h"
#include "../out/out.h"
#include "../out/lbl.h"
#include "../out/ctrl.h"
#include "../pack.h"
#include "../sue.h"
#include "../funcargs.h"

#include "__builtin_va.h"

#include "../tokenise.h"
#include "../tokconv.h"
#include "../parse_type.h"
#include "../type_is.h"
#include "../type_nav.h"

#include "../parse_expr.h"
#include "__builtin_va.h"

#include "../ops/expr_identifier.h"
#include "../ops/expr_assign.h"
#include "../ops/expr_struct.h"
#include "../ops/expr_deref.h"
#include "../ops/expr_val.h"
#include "../ops/expr_op.h"
#include "../ops/expr_funcall.h"
#include "../ops/expr_cast.h"

static int va_type_check(
		expr *va_l, expr *in, symtable *stab, int expect_decay)
{
	/* we need to check decayed, since we may have
	 * f(va_list l)
	 * aka
	 * f(__builtin_va_list *l) [the array has decayed]
	 */
	enum type_cmp cmp;
	type *va_list_ty = type_nav_va_list(cc1_type_nav, stab);

	if(!symtab_func(stab))
		die_at(&in->where, "%s() outside a function",
				BUILTIN_SPEL(in));

	if(expect_decay)
		va_list_ty = type_decay(va_list_ty);

	cmp = type_cmp(va_l->tree_type, va_list_ty, 0);

	if(!(cmp & TYPE_EQUAL_ANY)){
		warn_at_print_error(&va_l->where,
				"first argument to %s should be a va_list (not %s)",
				BUILTIN_SPEL(in), type_to_str(va_l->tree_type));
		fold_had_error = 1;
		return 0;
	}
	return 1;
}

static void va_ensure_variadic(expr *e, symtable *stab)
{
	funcargs *args = type_funcargs(symtab_func(stab)->ref);

	if(!args->variadic)
		die_at(&e->where, "%s in non-variadic function", BUILTIN_SPEL(e->expr));
}

static void add_assignment(
		char *member,
		expr *e,
		expr *va_l,
		stmt *assigns,
		where *w,
		symtable *symtab)
{
	expr *assign = expr_set_where(
			expr_new_assign(
				expr_set_where(
					expr_new_struct(
						expr_new_deref(va_l),
						1 /* -> since it's *(e) */,
						expr_set_where(
							expr_new_identifier(member),
							w)),
					w),
				e),
			w);
	dynarray_add(
			&assigns->bits.stmt_and_decls,
			stmt_set_where(
				expr_to_stmt(assign, symtab),
				w));
}

static void add_assignment_value(
		char *member,
		size_t value,
		expr *va_l,
		stmt *assigns,
		where *w,
		symtable *symtab)
{
	add_assignment(member,
			expr_set_where(
				expr_new_val(value),
				w),
			va_l,
			assigns,
			w,
			symtab);
}

static void fold_va_start(expr *e, symtable *stab)
{
	expr *va_l;

	if(dynarray_count(e->funcargs) != 2)
		die_at(&e->where, "%s requires two arguments", BUILTIN_SPEL(e->expr));

	va_l = e->funcargs[0];
	fold_inc_writes_if_sym(va_l, stab);

	fold_expr_nodecay(e->funcargs[0], stab); /* prevent lval2rval */
	FOLD_EXPR(e->funcargs[1], stab);

	e->tree_type = type_nav_btype(cc1_type_nav, type_void);

	va_l = e->funcargs[0];
	if(!va_type_check(va_l, e->expr, stab, 0))
		return;

	va_ensure_variadic(e, stab);

	/* second arg check */
	{
		sym *second = NULL;
		decl **args = symtab_decls(symtab_func_root(stab));
		sym *arg = args[dynarray_count(args) - 1]->sym;
		expr *last_exp = expr_skip_all_casts(e->funcargs[1]); /* e.g. enum -> int casts */

		if(expr_kind(last_exp, identifier))
			second = last_exp->bits.ident.bits.ident.sym;

		if(second != arg)
			cc1_warn_at(&last_exp->where,
					builtin_va_start,
					"second parameter to va_start isn't last named argument");
	}

#ifndef UCC_VA_ABI
#define W(exp) expr_set_where((exp), &e->where)
	{
		stmt *assigns = stmt_set_where(
				stmt_new_wrapper(code, symtab_new(stab, &e->where)),
				&e->where);
		const int ws = platform_word_size();
		struct
		{
			unsigned gp, fp;
		} nargs = { 0, 0 };
		funcargs *const fa = type_funcargs(symtab_func(stab)->ref);

		funcargs_ty_calc(fa, &nargs.gp, &nargs.fp);

		/* need to set the offsets to act as if we've skipped over
		 * n call regs, since we may already have some arguments used
		 */
		add_assignment_value("gp_offset", nargs.gp * ws, va_l, assigns, &e->where, stab);
		add_assignment_value("fp_offset", (6 + nargs.fp) * ws, va_l, assigns, &e->where, stab);
		/* FIXME: x86_64::N_CALL_REGS_I reference above */

		add_assignment("reg_save_area", W(builtin_new_reg_save_area()), va_l, assigns, &e->where, stab);

		add_assignment("overflow_arg_area",
				W(expr_new_op2(op_plus,
					W(expr_new_cast(
						W(builtin_new_frame_address(0)),
						type_ptr_to(type_nav_btype(cc1_type_nav, type_nchar)),
						1)),
					/* *2 to step over saved-rbp and saved-ret */
					W(expr_new_val(ws * 2)))),
				va_l,
				assigns,
				&e->where,
				stab);


		fold_stmt(assigns);
		e->bits.variadic_setup = assigns;
	}
#undef ADD_ASSIGN
#undef ADD_ASSIGN_VAL
#undef W
#endif
}

static const out_val *builtin_gen_va_start(const expr *e, out_ctx *octx)
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
	out_push_zero(type_new_INTPTR_T());
	out_store();
#else
	out_comment(octx, "va_start() begin");
	gen_stmt(e->bits.variadic_setup, octx);
	out_comment(octx, "va_start() end");
	return out_new_noop(octx);
#endif
}

expr *parse_va_start(const char *ident, symtable *scope)
{
	/* va_start(__builtin_va_list &, identifier)
	 * second argument may be any expression - we don't use it
	 */
	expr *fcall = parse_any_args(scope);
	(void)ident;
	expr_mutate_builtin(fcall, va_start);
	return fcall;
}

static const out_val *va_arg_gen_read(
		const expr *const e,
		out_ctx *const octx,
		type *const ty,
		decl *const offset_decl, /* varies - float or integral */
		decl *const mem_reg_save_area,
		decl *const mem_overflow_arg_area)
{
	out_blk *blk_reg = out_blk_new(octx, "va_reg");
	out_blk *blk_stack = out_blk_new(octx, "va_stack");
	out_blk *blk_fin = out_blk_new(octx, "va_fin");

	/* FIXME: this needs to reference x86_64::N_CALL_REGS_{I,F} */
	const int fp = type_is_floating(ty);
	const unsigned max_reg_args_sz = 6 * 8 + (fp ? 16 * 16 : 0);
	const unsigned ws = platform_word_size();
	const out_val *valist, *gpoff_addr, *gpoff_val;
	const int VALIST_OFFSET_TYPE = type_uint;
	type *voidp = type_ptr_to(type_nav_btype(cc1_type_nav, type_void));
	type *valist_off_ty = type_nav_btype(cc1_type_nav, VALIST_OFFSET_TYPE);
	type *ty_long = type_nav_btype(cc1_type_nav, type_long);

	valist = gen_expr(e->lhs, octx);
	valist = out_change_type(octx, valist, voidp);

	out_val_retain(octx, valist);
	gpoff_addr = out_op(
			octx, op_plus,
			valist,
			out_new_l(
				octx, ty_long,
				offset_decl->bits.var.struct_offset));


	gpoff_addr = out_change_type(octx, gpoff_addr,
			type_ptr_to(valist_off_ty));

	out_val_retain(octx, gpoff_addr);

	gpoff_val = out_deref(octx, gpoff_addr);


	/* gpoff_val, gpoff_addr and valist live across blocks, but don't need to be
	 * spilt because we look after them and ensure they remain alive/valid and
	 * not clobbered (whether because of a function call or register pressure) */
	gpoff_val = out_val_blockphi_make(octx, gpoff_val, NULL);
	gpoff_addr = out_val_blockphi_make(octx, gpoff_addr, NULL);
	valist = out_val_blockphi_make(octx, valist, NULL);

	out_val_retain(octx, gpoff_val);
	out_ctrl_branch(
			octx,
			out_op(octx,
				op_lt,
				gpoff_val,
				out_new_l(octx, valist_off_ty, max_reg_args_sz)),
			blk_reg,
			blk_stack);

	/* now inserting into blk_reg */
	{
		const unsigned increment = fp ? 2 * ws : ws;
		const out_val *gp_off_plus, *membptr;
		const out_val *reg_save_area_value;

		/* increment either 8 for an integral, or 16 for a float argument
		 * since xmm0 are 128-bit registers, aka 16 byte
		 */
		out_val_retain(octx, gpoff_val);
		gp_off_plus =
			out_op(octx,
					op_plus,
					gpoff_val,
					out_new_l(octx, valist_off_ty, increment));

		out_store(octx, gpoff_addr, gp_off_plus);

		out_val_retain(octx, valist);
		membptr =
			out_change_type(
					octx,
					out_op(
						octx, op_plus,
						valist,
						out_new_l(
							octx, ty_long,
							mem_reg_save_area->bits.var.struct_offset)),
					type_ptr_to(voidp)); /* void *reg_save_area */

		reg_save_area_value = out_op(
				octx, op_plus,
				out_deref(octx, membptr),
				out_change_type(
					octx,
					gpoff_val, /* promote from unsigned to long/intptr_t */
					type_ptr_to(ty_long)));

		out_ctrl_transfer(octx, blk_fin, reg_save_area_value, &blk_reg, 1);
	}

	/* stack code */
	out_current_blk(octx, blk_stack);
	{
		const out_val *overflow_val;
		const out_val *overflow_addr =
			out_change_type(
					octx,
					out_op(
						octx, op_plus,
						out_change_type(octx, valist, voidp),
						out_new_l(
							octx, ty_long,
							mem_overflow_arg_area->bits.var.struct_offset)),
					type_ptr_to(type_ptr_to(valist_off_ty)));

		out_val_retain(octx, overflow_addr);

		overflow_val = out_deref(octx, overflow_addr);

		out_val_retain(octx, overflow_addr);
		out_store(
				octx,
				overflow_addr,
				out_op(
					octx, op_plus,
					out_change_type(octx, out_deref(octx, overflow_addr), voidp),
					out_new_l(octx, valist_off_ty, ws)));

		out_ctrl_transfer(octx, blk_fin, overflow_val, &blk_stack, 1);
	}

	out_current_blk(octx, blk_fin);
	return out_deref(
			octx,
			out_change_type(
				octx,
				/* now have a pointer to the right memory address: */
				out_ctrl_merge(octx, blk_reg, blk_stack),
				type_ptr_to(ty)));
}

static const out_val *builtin_gen_va_arg(const expr *e, out_ctx *octx)
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

	out_change_type(type_new_LONG_PTR());
	out_deref();
	/* &va, va */

	/* out_n_call_regs() has been revoked - UCC ABI is obsolete */
	out_push_l(type_new_LONG(), out_n_call_regs() - nargs);
	out_op(op_lt);
	/* &va, (<) */

	out_jfalse(lbl_else);
	/* &va */

	/* __builtin_frame_address(0) - nargs
	 * - multiply by pws is implicit - void *
	 */
	out_push_frame_ptr(0);
	out_change_type(type_new_LONG_PTR());
	out_push_l(type_new_INTPTR_T(), nargs);
	out_op(op_minus);
	/* &va, va_ptr */

	/* - (intptr_t)val[0]++  */
	out_swap(); /* pull &val to the top */

	/* va_ptr, &va */
	out_dup();
	out_change_type(type_new_LONG_PTR());
	/* va_ptr, (long *)&va, (int *)&va */

	out_deref();
	/* va_ptr, &va, va */

	out_push_l(type_new_INTPTR_T(), 1);
	out_op(op_plus); /* val[0]++ */
	/* va_ptr, &va, (va+1) */
	out_store();
	/* va_ptr, (va+1) */

	out_op(op_minus);
	/* va_ptr - (va+1) */
	/* va_ptr - va - 1 = va_ptr_arg-1 */

	EOF_WHERE(&e->where,
		out_change_type(type_new_ptr(e->tree_type, qual_none));
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
	out_push_l(type_new_LONG(), type_size(e->bits.tref, NULL));
	/* 0 - abi.c's gen_reg. this is temporary until we have builtin_va_arg proper */
	out_push_zero(type_new_INT());
	gen_expr(e->lhs);

	extern void *funcargs_new(); /* XXX: temporary hack for the call */

	out_call(3, type_new_ptr(e->bits.tref, qual_none),
			type_new_func(type_new_VOID(), funcargs_new()));

	out_deref(); /* __va_arg returns a pointer to the stack location of the argument */
#else
	{
		type *const ty = e->bits.va_arg_type;

		if(type_is_s_or_u(ty)){
			ICE("TODO: s/u/e va_arg");
stack:
			ICE("TODO: stack __builtin_va_arg()");

		}else{
			const btype *typ = type_get_type(ty);
			const int fp = typ && type_floating(typ->primitive);
			struct_union_enum_st *sue_va;

			if(typ && typ->primitive == type_ldouble)
				goto stack;

			sue_va = type_next(
						type_nav_va_list(cc1_type_nav, NULL)
					)->bits.type->sue;

#define VA_DECL(nam) \
			decl *mem_ ## nam = struct_union_member_find(sue_va, #nam, NULL, NULL)
			VA_DECL(gp_offset);
			VA_DECL(fp_offset);
			VA_DECL(reg_save_area);
			VA_DECL(overflow_arg_area);

			return va_arg_gen_read(
					e, octx,
					ty,
					fp ? mem_fp_offset : mem_gp_offset,
					mem_reg_save_area,
					mem_overflow_arg_area);
		}
	}

#endif
}

static void fold_va_arg(expr *e, symtable *stab)
{
	type *const ty = e->bits.va_arg_type;
	type *to;
	const char *warning = NULL;

	FOLD_EXPR(e->lhs, stab);
	fold_type(ty, stab);

	if(!va_type_check(e->lhs, e->expr, stab, 1)){
		e->tree_type = type_nav_btype(cc1_type_nav, type_int);
		return;
	}

	if(type_is_promotable(ty, &to)){
		warning = "";
	}else if(type_is_enum(ty)){
		warning = "when enums are short ";
		to = type_nav_btype(cc1_type_nav, type_int);
	}

	if(warning){
		char tbuf[TYPE_STATIC_BUFSIZ];

		cc1_warn_at(&e->where,
				builtin_va_arg,
				"va_arg(..., %s) has undefined behaviour %s- promote to %s",
				type_to_str(ty), warning, type_to_str_r(tbuf, to));
	}

	e->tree_type = ty;

#ifdef UCC_VA_ABI
	/* finally store the number of arguments to this function */
	e->bits.n = dynarray_count(
			type_funcargs(
				curdecl_func->ref)->arglist)
#endif
}

expr *parse_va_arg(const char *ident, symtable *scope)
{
	/* va_arg(list, type) */
	expr *fcall = expr_new_funcall();
	expr *list = PARSE_EXPR_NO_COMMA(scope, 0);
	type *ty;

	(void)ident;

	EAT(token_comma);
	ty = parse_type(0, scope);

	fcall->lhs = list;
	fcall->bits.va_arg_type = ty;

	expr_mutate_builtin(fcall, va_arg);

	return fcall;
}

static const out_val *builtin_gen_va_end(const expr *e, out_ctx *octx)
{
	out_val_release(octx, gen_expr(e->funcargs[0], octx));
	return out_new_noop(octx);
}

static void fold_va_end(expr *e, symtable *stab)
{
	e->tree_type = type_nav_btype(cc1_type_nav, type_void);

	if(dynarray_count(e->funcargs) != 1){
		warn_at_print_error(&e->where, "%s requires one argument", BUILTIN_SPEL(e->expr));
		fold_had_error = 1;
		return;
	}

	FOLD_EXPR(e->funcargs[0], stab);
	if(!va_type_check(e->funcargs[0], e->expr, stab, 1))
		return;

	/*va_ensure_variadic(e, stab); - va_end can be anywhere */
}

expr *parse_va_end(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);

	(void)ident;
	expr_mutate_builtin(fcall, va_end);
	return fcall;
}

static const out_val *builtin_gen_va_copy(const expr *e, out_ctx *octx)
{
	out_val_release(octx, gen_expr(e->lhs, octx));
	return out_new_noop(octx);
}

static void fold_va_copy(expr *e, symtable *stab)
{
	int i;

	e->tree_type = type_nav_btype(cc1_type_nav, type_void);

	if(dynarray_count(e->funcargs) != 2){
		warn_at_print_error(&e->where, "%s requires two arguments", BUILTIN_SPEL(e->expr));
		fold_had_error = 1;
		return;
	}

	for(i = 0; i < 2; i++){
		FOLD_EXPR(e->funcargs[i], stab);
		if(!va_type_check(e->funcargs[i], e->expr, stab, 1))
			return;
	}

	/* (*a) = (*b) */
	e->lhs = builtin_new_memcpy(
			expr_new_deref(e->funcargs[0]),
			expr_new_deref(e->funcargs[1]),
			type_size(type_nav_va_list(cc1_type_nav, stab), &e->where));

	FOLD_EXPR(e->lhs, stab);
}

expr *parse_va_copy(const char *ident, symtable *scope)
{
	expr *fcall = parse_any_args(scope);
	(void)ident;
	expr_mutate_builtin(fcall, va_copy);
	return fcall;
}
