#include <stddef.h>
#include <assert.h>

#include "../util/dynarray.h"

#include "expr.h"
#include "stmt.h"
#include "gen_asm.h"
#include "cc1.h" /* fopt_mode */

#include "inline.h"

#include "cc1_out_ctx.h"

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

#define CANT_INLINE(reason, nam) do{ \
		if(fopt_mode & FOPT_VERBOSE_ASM) \
			out_comment(octx, "can't inline %s: %s", nam, reason); \
		return NULL; \
	}while(0)

static decl *expr_to_declref(expr *e, out_ctx *octx)
{
	e = expr_skip_casts(e);

	if(expr_kind(e, identifier)){
		if(e->bits.ident.type == IDENT_NORM)
			return e->bits.ident.bits.ident.sym->decl;
		else
			CANT_INLINE("not normal identifier", "enum");

	}else if(expr_kind(e, block)){
		return e->bits.block.sym->decl;

	}else{
		CANT_INLINE("not identifier", e->f_str());
	}
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

const out_val *try_gen_inline_func(
		expr *call_expr,
		const out_val *fn, const out_val **args,
		out_ctx *octx)
{
	struct cc1_out_ctx *cc1_octx;
	decl *decl_fn;
	stmt *fn_code;
	symtable *arg_symtab;
	decl **diter;
	const out_val *inlined_ret;

	decl_fn = expr_to_declref(call_expr, octx);

	if(!(fn_code = decl_fn->bits.func.code))
		CANT_INLINE("can't see func code", decl_fn->spel);

	arg_symtab = DECL_FUNC_ARG_SYMTAB(decl_fn);

	/* can't do functions where the argument count != param count */
	if(dynarray_count(args) != dynarray_count(arg_symtab->decls))
		CANT_INLINE("variadic or unspec arg function", decl_fn->spel);

	for(diter = arg_symtab->decls; diter && *diter; diter++)
		if((*diter)->sym->nwrites)
			CANT_INLINE("sym written or addressed", decl_fn->spel);
	/* TODO: ^ name the above sym/decl */

	cc1_octx = cc1_out_ctx_or_new(octx);
	if(cc1_octx->inline_.depth >= INLINE_DEPTH_MAX)
		CANT_INLINE("inline depth", decl_fn->spel);

	if(!heuristic_should_inline(decl_fn, fn_code, arg_symtab->children[0]))
		CANT_INLINE("heuristic denied", decl_fn->spel);

	cc1_octx->inline_.depth++;
	{
		/* we don't use the call expr/value */
		out_val_consume(octx, fn);

		inlined_ret = gen_inline_func(
				arg_symtab, fn_code, args, cc1_octx, octx);
	}
	cc1_octx->inline_.depth--;

	return inlined_ret;
#undef CANT_INLINE
}
