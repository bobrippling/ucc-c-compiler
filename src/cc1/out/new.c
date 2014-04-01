#include <stdlib.h>
#include <assert.h>

#include "../type.h"
#include "../type_nav.h"
#include "../type_is.h"
#include "../sym.h"

#include "out.h" /* this file defs */
#include "val.h"
#include "ctx.h"

#include <stdio.h>
#define TODO() fprintf(stderr, "TODO: %s\n", __func__)

out_val *out_new_blk_addr(out_ctx *octx, out_blk *blk)
{
	TODO();
	return 0;
}

out_val *out_new_frame_ptr(out_ctx *octx, int nframes)
{
	TODO();
	return 0;
}

out_val *out_new_num(out_ctx *octx, type *ty, const numeric *n)
{
	out_val *v = v_new(octx, ty);

	if(n->suffix & VAL_FLOATING){
		v->type = V_CONST_F;
		v->bits.val_f = n->val.f;
	}else{
		v->type = V_CONST_I;
		v->bits.val_i = n->val.i;
	}

	return v;
}

out_val *out_new_l(out_ctx *octx, type *ty, long val)
{
	numeric n;

	n.val.i = val;
	n.suffix = 0;

	return out_new_num(octx, ty, &n);
}

out_val *out_new_lbl(out_ctx *octx, const char *s, int pic)
{
	TODO();
	return 0;
}

out_val *out_new_nan(out_ctx *octx, type *ty)
{
	TODO();
	return 0;
}

out_val *out_new_noop(out_ctx *octx)
{
	TODO();
	return 0;
}

out_val *out_new_overflow(out_ctx *octx)
{
	TODO();
	return 0;
}

out_val *out_new_reg_save_ptr(out_ctx *octx)
{
	TODO();
	return 0;
}

out_val *out_new_sym(out_ctx *octx, sym *sym)
{
	type *ty = type_ptr_to(sym->decl->ref);

	switch(sym->type){
		case sym_global:
label:
		{
			out_val *v = v_new(octx, ty);
			v->type = V_LBL;
			v->bits.lbl.str = decl_asm_spel(sym->decl);
			v->bits.lbl.pic = 1;
			v->bits.lbl.offset = 0;
			return v;
		}

		case sym_arg:
			return v_new_sp3(octx, NULL, ty, sym->loc.arg_offset);

		case sym_local:
		{
			decl *d = sym->decl;

			if(type_is(d->ref, type_func))
				goto label;

			if((d->store & STORE_MASK_STORE) == store_register && d->spel_asm){
				fprintf(stderr, "TODO: %s asm(\"%s\")", decl_to_str(d), d->spel_asm);
				assert(0);
			}

			/* sym offsetting takes into account the stack growth direction */
			return v_new_sp3(octx, NULL, ty,
					-(long)(sym->loc.stack_pos + octx->stack_local_offset));
		}
	}

	assert(0);
}

out_val *out_new_sym_val(out_ctx *octx, sym *sym)
{
	return out_deref(octx, out_new_sym(octx, sym));
}

out_val *out_new_zero(out_ctx *octx, type *ty)
{
	TODO();
	return 0;
}
