#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>

#include "../util/dynarray.h"
#include "../util/warn.h"

#include "expr.h"
#include "stmt.h"
#include "gen_asm.h"
#include "cc1.h" /* fopt_mode */

#include "inline.h"

#include "cc1_out_ctx.h"

#define INLINE_DEPTH_MAX 5
#define INLINE_MAX_STACK_BYTES 256
#define INLINE_MAX_STMTS 10

static void inline_vars_push(
		struct cc1_inline *new, struct cc1_inline *save)
{
	memcpy_safe(save, new);
}

static void inline_vars_pop(
		struct cc1_inline *current, struct cc1_inline *saved)
{
	/* block memory management is handled by the out/backend
	 * we just free our array of them */
	dynarray_free(out_blk **, &current->rets, NULL);
	memcpy_safe(current, saved);
}

static const out_val *merge_inline_rets(
		struct cc1_inline *inline_, out_ctx *octx)
{
	if(inline_->rets)
		return out_ctrl_merge_n(octx, inline_->rets);
	return out_new_noop(octx);
}

static const out_val *gen_inline_func(
		symtable *arg_symtab,
		stmt *func_code, const out_val **args,
		struct cc1_out_ctx *cc1_octx,
		out_ctx *octx)
{
	struct cc1_inline saved;
	decl **diter;
	size_t i;
	const out_val *merged_ret;

	if(!cc1_octx->sym_inline_map)
		cc1_octx->sym_inline_map = dynmap_new(sym *, NULL, sym_hash);

	inline_vars_push(&cc1_octx->inline_, &saved);
	cc1_octx->inline_.phi = out_blk_new(octx, "inline_phi");

	/* got a handle on the code, map the identifiers to our argument
	 * expression/values, and generate */
	for(i = 0, diter = arg_symtab->decls; diter && *diter; i++, diter++){
		sym *s = (*diter)->sym;
		const out_val *prev;

		assert(args[i]);
		assert(s && s->type == sym_arg);

		prev = dynmap_set(sym *, const out_val *,
				cc1_octx->sym_inline_map,
				s, args[i]);

		assert(!prev);
	}

	gen_stmt(func_code, octx);

	for(diter = arg_symtab->decls; diter && *diter; diter++){
		sym *s = (*diter)->sym;
		const out_val *arg;

		arg = dynmap_rm(sym *, const out_val *,
				cc1_octx->sym_inline_map, s);

		out_val_release(octx, arg);
	}

	out_ctrl_transfer_make_current(octx, cc1_octx->inline_.phi);

	merged_ret = merge_inline_rets(&cc1_octx->inline_, octx);
	inline_vars_pop(&cc1_octx->inline_, &saved);

	return merged_ret;
}

void inline_ret_add(out_ctx *octx, const out_val *v)
{
	out_blk *mergee = NULL;
	struct cc1_out_ctx *cc1_octx = cc1_out_ctx_or_new(octx);

	out_ctrl_transfer(
			octx,
			cc1_octx->inline_.phi,
			v, v ? &mergee : NULL);

	if(mergee)
		dynarray_add(&cc1_octx->inline_.rets, mergee);
}

static int heuristic_should_inline(
		decl *fndecl, stmt *fncode, symtable *symtab)
{
	unsigned nstmts = 0;

	/* if it's marked inline, inline it
	 * if there's more than X stack space, deny
	 * if there's too many statements, deny
	 */
	if(fndecl->store & store_inline)
		return 1;

	if(symtab->auto_total_size > INLINE_MAX_STACK_BYTES)
		return 0;

	stmt_walk(fncode, stmts_count, NULL, &nstmts);
	if(nstmts > INLINE_MAX_STMTS)
		return 0;

	return 1;
}

struct inline_outs
{
	decl *fndecl;
	symtable *arg_symtab;
	stmt *fncode;
	struct cc1_out_ctx *cc1_octx;
};

static int check_and_ret_inline(
		expr *call_expr, const char **why, out_ctx *octx,
		struct inline_outs *iouts, int nargs)
{
	decl **diter;

	iouts->fndecl = expr_to_declref(call_expr, why);
	if(!iouts->fndecl)
		return 0;

	if(!(iouts->fncode = iouts->fndecl->bits.func.code)){
		*why = "can't see function";
		return 0;
	}

	if(attribute_present(iouts->fndecl, attr_noinline)){
		/* jumping to noinline means __attribute((noinline, always_inline))
		 * will always cause an error */
		*why = "has noinline attribute";
		return 0;
	}

	iouts->arg_symtab = DECL_FUNC_ARG_SYMTAB(iouts->fndecl);

	/* can't do functions where the argument count != param count */
	if(nargs != dynarray_count(iouts->arg_symtab->decls)){
		*why = "variadic or unspec arg function";
		return 0;
	}

	for(diter = iouts->arg_symtab->decls; diter && *diter; diter++){
		if((*diter)->sym->nwrites){
			*why = "argument written or addressed";
			return 0;
		}
	}

	if(octx){
		struct cc1_out_ctx *cc1_octx = cc1_out_ctx_or_new(octx);
		iouts->cc1_octx = cc1_octx;
		if(cc1_octx->inline_.depth >= INLINE_DEPTH_MAX){
			*why = "recursion depth";
			return 0;
		}
	}

	if(!attribute_present(iouts->fndecl, attr_always_inline)
	&& !heuristic_should_inline(iouts->fndecl,
		iouts->fncode, iouts->arg_symtab->children[0]))
	{
		*why = "heuristic denied";
		return 0;
	}

	return 1;
}

int inline_func_possible(expr *call_expr, int nargs, const char **why)
{
	struct inline_outs iouts = { 0 };

	return check_and_ret_inline(
			call_expr, why, NULL,
			&iouts, nargs);
}

const out_val *inline_func_try_gen(
		expr *call_expr,
		const out_val *fn, const out_val **args,
		out_ctx *octx)
{
	const out_val *inlined_ret;

	int can_inline;
	const char *why;

	struct inline_outs iouts = { 0 };

	can_inline = check_and_ret_inline(
			call_expr, &why, octx,
			&iouts, dynarray_count(args));

	if(!can_inline){
		if(fopt_mode & FOPT_VERBOSE_ASM)
			out_comment(octx, "can't inline call: %s", why);

		if(attribute_present(iouts.fndecl, attr_always_inline))
			warn_at(&call_expr->where, "couldn't always_inline call: %s", why);

		return NULL;
	}

	iouts.cc1_octx->inline_.depth++;
	{
		/* we don't use the call expr/value */
		out_val_consume(octx, fn);

		inlined_ret = gen_inline_func(
				iouts.arg_symtab,
				iouts.fncode,
				args,
				iouts.cc1_octx,
				octx);
	}
	iouts.cc1_octx->inline_.depth--;

	return inlined_ret;
}
