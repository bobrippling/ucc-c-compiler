#include <stddef.h>
#include <assert.h>

#include "../util/platform.h"

#include "decl.h"
#include "type_is.h"
#include "type_nav.h"
#include "gen_asm.h"

#include "out/out.h"
#include "out/val.h"

#include "vla.h"

unsigned vla_space()
{
	/*  T *ptr; size_t size;   */
	return platform_word_size() * 2;
}

static const out_val *vla_gen_size(
		type *t, out_ctx *octx,
		type *const arith_ty)
{
	int mul;

	switch(t->type){
		case type_btype:
			return out_new_l(octx, arith_ty, type_size(t, NULL));

		case type_ptr:
		case type_block:
			mul = platform_word_size();
			break;

		case type_array:
			if(t->bits.array.is_vla){
				return out_op(octx, op_multiply,
						vla_gen_size(type_next(t), octx, arith_ty),
						gen_expr(t->bits.array.size, octx));

			}else if(t->bits.array.size){
				mul = const_fold_val_i(t->bits.array.size);
			}else{
				assert(0 && "empty array?");
			}
			break;

		case type_func:
			mul = 1;
			break;

		case type_tdef:
			assert(0 && "TODO tdef");

		case type_auto:
			assert(0 && "__auto_type in vla");

		case type_cast:
		case type_attr:
		case type_where:
			return vla_gen_size(type_skip_all(t), octx, arith_ty);
	}

	return out_op(octx, op_multiply,
			out_new_l(octx, arith_ty, mul),
			vla_gen_size(type_next(t), octx, arith_ty));
}

void vla_alloc(decl *d, out_ctx *octx)
{
	sym *s = d->sym;
	struct
	{
		long sz, ptr;
	} locns;
	const out_val *v_sz;
	const out_val *v_ptr;
	type *sizety = type_nav_btype(cc1_type_nav, type_long);
	type *ptrsizety = type_ptr_to(sizety);

	assert(s && "no sym for vla");

	locns.sz = s->loc.stack_pos;
	locns.ptr = locns.sz + platform_word_size();

	v_sz = vla_gen_size(d->ref, octx, sizety);

	v_sz = out_cast(octx, v_sz, sizety, 0);

	out_val_retain(octx, v_sz);
	out_store(octx,
			v_new_bp3_below(octx, NULL, ptrsizety, locns.sz),
			v_sz);

	v_ptr = out_alloca_push(octx, v_sz, type_align(d->ref, NULL));
	out_store(octx,
			v_new_bp3_below(octx, NULL, ptrsizety, locns.ptr),
			v_ptr);
}

const out_val *vla_address(decl *d, out_ctx *octx)
{
	type *ptr_to_vla_ty = type_ptr_to(type_ptr_to(d->ref));

	const out_val *read_ptr = out_deref(octx,
			v_new_bp3_below(octx,
				NULL, ptr_to_vla_ty,
				d->sym->loc.stack_pos + platform_word_size()));

	return read_ptr;
}
