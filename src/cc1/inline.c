#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <assert.h>

#include "../util/dynarray.h"
#include "../util/warn.h"
#include "../util/alloc.h"

#include "expr.h"
#include "stmt.h"
#include "gen_asm.h"
#include "cc1.h" /* fopt_mode */

/* inline emission */
#include "out/dbg.h"
#include "out/lbl.h"
#include "out/dbg_lbl.h"

/* old-func detection */
#include "funcargs.h"
#include "type_is.h"

#include "inline.h"

#include "cc1_out_ctx.h"

#define INLINE_DEPTH_MAX 5
#define INLINE_MAX_STACK_BYTES 128
#define INLINE_VLA_COST 64
#define INLINE_MAX_STMTS 4

struct inline_outs
{
	decl *fndecl;
	symtable *arg_symtab;
	stmt *fncode;
	struct cc1_out_ctx *cc1_octx;
};

struct inline_sym_map
{
	const out_val *sym_outval;
	const out_val *map_val;
};


static void inline_vars_push(
		struct cc1_out_ctx *cc1_octx, struct cc1_inline *save,
		dynmap **const nested_label_map)
{
	struct cc1_inline *const new = &cc1_octx->inline_;

	memcpy_safe(save, new);
	new->phi = NULL;
	new->rets = NULL;
	/* keep depth */

	/* current label context is saved */
	*nested_label_map = cc1_octx->label_to_blk;
	cc1_octx->label_to_blk = NULL;
}

static void inline_vars_pop(
		struct cc1_out_ctx *cc1_octx, struct cc1_inline *saved,
		dynmap **const nested_label_map)
{
	struct cc1_inline *const current = &cc1_octx->inline_;

	/* block memory management is handled by the out/backend
	 * we just free our array of them */
	dynarray_free(out_blk **, current->rets, NULL);
	memcpy_safe(current, saved);

	/* restore current label context */
	dynmap_free(cc1_octx->label_to_blk);
	cc1_octx->label_to_blk = *nested_label_map;
	*nested_label_map = NULL;
}

static const out_val *merge_inline_rets(
		struct cc1_inline *inline_, out_ctx *octx)
{
	if(inline_->rets)
		return out_ctrl_merge_n(octx, inline_->rets);
	return out_new_noop(octx);
}

static const out_val *inline_arg_to_lvalue(
		out_ctx *octx, const out_val *rval, type *const ty)
{
	const out_val *space = out_aalloct(octx, ty);

	out_val_retain(octx, space);
	out_store(octx, space, rval);

	return space;
}

static void inline_sym_map_save(
		symtable *arg_symtab,
		const out_val **args,
		struct inline_sym_map *pushed_vals,
		struct cc1_out_ctx *cc1_octx,
		out_ctx *octx)
{
	size_t i;
	decl **diter;

	for(i = 0, diter = symtab_decls(arg_symtab); diter && *diter; i++, diter++){
		sym *s = (*diter)->sym;

		assert(args[i]);
		assert(s && s->type == sym_arg);

		/* if we're doign a (mutually-)recursive inline, we're replacing the
		 * _exact_ symbol by a new value. we need to push/pop it */
		pushed_vals[i].sym_outval = sym_outval(s);

		/* if the symbol is addressed we need to spill it */
		if(s->nwrites || out_is_nonconst_temporary(args[i])){
			/* registers can't persist across inlining in the case of
			 * function calls, etc etc - need to spill, hence
			 * non-const temporary */
			sym_setoutval(s, inline_arg_to_lvalue(octx, args[i], s->decl->ref));

			pushed_vals[i].map_val = NULL;
		}else{
			const out_val *was_set;

			was_set = dynmap_set(sym *, const out_val *,
					cc1_octx->sym_inline_map,
					s, args[i]);

			/* no outval, but a value for the lvalue2rvalue uses of this sym */
			sym_setoutval(s, NULL);

			pushed_vals[i].map_val = was_set;
		}

		if(cc1_gdebug){
			/* sym_outval() may be null, in which case the debugger
			 * will show "argument optimised out" etc etc.. */
			out_dbg_emit_decl(octx, *diter, sym_outval(s));
		}

		/* generate vla side-effects */
		gen_vla_arg_sideeffects(*diter, octx);
	}
}

static void inline_sym_map_restore(
		symtable *arg_symtab,
		struct inline_sym_map *pushed_vals,
		struct cc1_out_ctx *cc1_octx,
		out_ctx *octx)
{
	size_t i;
	decl **diter;

	for(i = 0, diter = symtab_decls(arg_symtab); diter && *diter; i++, diter++){
		sym *s = (*diter)->sym;
		const out_val *v;

		if((v = sym_outval(s))){
			/* lvalue case */
			out_val_release(octx, v);
			assert(!pushed_vals[i].map_val);
		}else{
			/* rvalue case */
			const out_val *arg;

			if(pushed_vals[i].map_val){
				arg = dynmap_set(sym *, const out_val *,
						cc1_octx->sym_inline_map, s, pushed_vals[i].map_val);
			}else{
				arg = dynmap_rm(sym *, const out_val *,
						cc1_octx->sym_inline_map, s);
			}

			out_val_release(octx, arg);
		}

		/* restore previous outval */
		sym_setoutval(s, pushed_vals[i].sym_outval);
	}
}

static const out_val *gen_inline_func(
		const struct inline_outs *const iouts,
		const out_val **args,
		struct cc1_out_ctx *cc1_octx,
		out_ctx *octx, const where *call_loc)
{
	symtable *const arg_symtab = iouts->arg_symtab;
	stmt *const func_code = iouts->fncode;
	struct cc1_inline saved;
	const out_val *merged_ret;
	const size_t nargs = dynarray_count(args);
	struct inline_sym_map *pushed_vals = umalloc(nargs * sizeof *pushed_vals);

	struct out_dbg_lbl *dbg_endlbl = NULL;
	dynmap *nested_label_map;

	if(!cc1_octx->sym_inline_map)
		cc1_octx->sym_inline_map = dynmap_new(sym *, NULL, sym_hash);

	inline_vars_push(cc1_octx, &saved, &nested_label_map);
	cc1_octx->inline_.phi = out_blk_new(octx, "inline_phi");

	if(cc1_gdebug){
		struct out_dbg_lbl *dbg_startlbl;
		char *dbg_lbls[2];

		dbg_lbls[0] = out_label_code("dbg_inline_start");
		dbg_lbls[1] = out_label_code("dbg_inline_end");

		/* free/release ownership of dbg_lbls[0 ... 1] */
		out_dbg_label_push(octx, dbg_lbls, &dbg_startlbl, &dbg_endlbl);

		out_dbg_inlined_call(octx,
				iouts->fndecl,
				dbg_startlbl,
				dbg_endlbl,
				call_loc);

		RELEASE(dbg_startlbl);
	}

	inline_sym_map_save(arg_symtab, args, pushed_vals, cc1_octx, octx);

	gen_func_stmt(func_code, octx);

	inline_sym_map_restore(arg_symtab, pushed_vals, cc1_octx, octx);

	if(cc1_gdebug){
		out_dbg_label_pop(octx, dbg_endlbl);
		out_dbg_inline_end(octx);
	}

	free(pushed_vals), pushed_vals = NULL;

	out_ctrl_transfer_make_current(octx, cc1_octx->inline_.phi);

	merged_ret = merge_inline_rets(&cc1_octx->inline_, octx);
	inline_vars_pop(cc1_octx, &saved, &nested_label_map);

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
		decl *fndecl, stmt *fncode, symtable *symtab, out_ctx *octx)
{
	unsigned nstmts = 0;
	unsigned new_stack;

	/* as with clang and gcc, -fno-inline-functions affects just the heuristic
	 * __attribute((always_inline)) overrides it */
	if((fopt_mode & FOPT_INLINE_FUNCTIONS) == 0)
		return 0;

	/* if it's marked inline, inline it
	 * if there's more than X stack space, deny
	 * if there's too many statements, deny
	 */
	if(fndecl->store & store_inline)
		return 1;

	new_stack = symtab_decl_bytes(symtab, INLINE_VLA_COST) + out_current_stack(octx);

	if(new_stack > INLINE_MAX_STACK_BYTES)
		return 0;

	stmt_walk(fncode, stmts_count, NULL, &nstmts);
	if(nstmts > INLINE_MAX_STMTS)
		return 0;

	return 1;
}

static stmt *try_resolve_val_to_func(
		out_ctx *octx, const out_val *fnval, decl **out_decl)
{
	/* no code - may be a forward decl, or may be a function pointer.
	 * if it's a function pointer, try to see if the backend has
	 * resolved it for us */
	struct cc1_out_ctx **pcc1_octx;

	pcc1_octx = cc1_out_ctx(octx);
	if(*pcc1_octx){
		const char *lbl = out_get_lbl(fnval);

		if(lbl){
			struct cc1_out_ctx *cc1_octx = *pcc1_octx;

			*out_decl = dynmap_get(
					const char *, decl *,
					cc1_octx->spel_to_fndecl,
					lbl);

			if(*out_decl)
				return decl_impl(*out_decl)->bits.func.code;
		}
	}
	return NULL;
}

ucc_nonnull()
static const char *check_and_ret_inline(
		expr *maybe_call_expr, decl *maybe_decl,
		out_ctx *octx,
		const out_val *fnval,
		struct inline_outs *iouts, int nargs)
{
	funcargs *fargs;
	const char *why;
	struct cc1_out_ctx *cc1_octx;
	decl **arg_iter;

	if(maybe_decl){
		iouts->fndecl = maybe_decl;
	}else{
		iouts->fndecl = expr_to_declref(maybe_call_expr, &why);
		if(!iouts->fndecl)
			return why;
	}

	iouts->fndecl = decl_impl(iouts->fndecl);

	if(iouts->fndecl->bits.func.contains_static_label_addr)
		return "function contains static-address-of-label expression";

	/* check for noinline before we potentially change the decl */
	if(attribute_present(iouts->fndecl, attr_noinline)){
		/* checking noinline first means __attribute((noinline, always_inline))
		 * will always cause an error */
		return "function has noinline attribute";
	}

	if(attribute_present(iouts->fndecl, attr_weak))
		return "weak-function overridable at link time";

	if(!(iouts->fncode = iouts->fndecl->bits.func.code)){
		/* may change the decl from fnptr -> function */
		iouts->fncode = try_resolve_val_to_func(octx, fnval, &iouts->fndecl);

		if(!iouts->fncode){
			/* see if the decl was later completed */
			return "can't see function code";
		}

		/* no need to check noinline attribute - gcc and clang
		 * disallow inline attributes on function pointers */
	}

	iouts->arg_symtab = DECL_FUNC_ARG_SYMTAB(iouts->fndecl);
	fargs = type_funcargs(iouts->fndecl->ref);

	if(fargs->variadic){
		return "call to variadic function";
	}

	/* can't do functions where the argument count != param count */
	if(funcargs_is_old_func(fargs)
	|| nargs != dynarray_count(symtab_decls(iouts->arg_symtab)))
	{
		return "call to function with unspecified arguments";
	}

	for(arg_iter = fargs->arglist; arg_iter && *arg_iter; arg_iter++){
		/* f(int [vla][3][2]) is fine due to function argument decay */
		decl *d = *arg_iter;
		type *t = type_is_ptr(d->ref);

		if(t && type_is_variably_modified(t))
			return "argument with variably modified type";
	}

	cc1_octx = cc1_out_ctx_or_new(octx);
	iouts->cc1_octx = cc1_octx;
	if(cc1_octx->inline_.depth >= INLINE_DEPTH_MAX){
		return "recursion too deep";
	}

	if(!attribute_present(iouts->fndecl, attr_always_inline)
	&& !heuristic_should_inline(iouts->fndecl,
		iouts->fncode, iouts->arg_symtab->children[0], octx))
	{
		return "heuristic denied";
	}

	return NULL;
}

const out_val *inline_func_try_gen(
		expr *maybe_call_expr, decl *maybe_decl,
		const out_val *fnval,
		const out_val **args,
		out_ctx *octx,
		const char **whynot, const where *call_loc)
{
	const out_val *inlined_ret;
	struct inline_outs iouts = { 0 };

	*whynot = check_and_ret_inline(
			maybe_call_expr, maybe_decl, octx, fnval,
			&iouts, dynarray_count(args));

	if(*whynot){
		if(fopt_mode & FOPT_VERBOSE_ASM)
			out_comment(octx, "can't inline call: %s", *whynot);
		return NULL;
	}

	iouts.cc1_octx->inline_.depth++;
	{
		/* we don't use the call expr/value */
		out_val_consume(octx, fnval);

		inlined_ret = gen_inline_func(
				&iouts,
				args,
				iouts.cc1_octx,
				octx, call_loc);
	}
	iouts.cc1_octx->inline_.depth--;

	return inlined_ret;
}
