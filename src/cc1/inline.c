#include <stddef.h>
#include <assert.h>

#include "../util/dynarray.h"

#include "expr.h"
#include "stmt.h"
#include "gen_asm.h"
#include "cc1.h" /* fopt_mode */

#include "type_nav.h" /* temporary - for out_new_int(0) */

#include "inline.h"

#include "cc1_out_ctx.h"

static const out_val *gen_inline_func(
		symtable *arg_symtab,
		stmt *func_code, const out_val **args,
		out_ctx *octx)
{
	struct cc1_out_ctx *cc1_octx = cc1_out_ctx_or_new(octx);
	decl **diter;
	size_t i;

	if(!cc1_octx->sym_inline_map)
		cc1_octx->sym_inline_map = dynmap_new(sym *, NULL, sym_hash);

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

	/* FIXME: placeholder until return is sorted: */
	return out_new_zero(octx, type_nav_btype(cc1_type_nav, type_int));
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

const out_val *try_gen_inline_func(
		expr *call_expr,
		const out_val *fn, const out_val **args,
		out_ctx *octx)
{
	decl *decl_fn;
	stmt *fn_code;
	symtable *arg_symtab;
	decl **diter;

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

	/* we don't use the call expr/value */
	out_val_consume(octx, fn);

	return gen_inline_func(arg_symtab, fn_code, args, octx);
#undef CANT_INLINE
}
