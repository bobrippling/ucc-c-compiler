#include <stddef.h>
#include <assert.h>

#include "../util/platform.h"

#include "decl.h"
#include "type_is.h"
#include "type_nav.h"
#include "gen_asm.h"

#include "out/out.h"
#include "out/val.h"
#include "out/ctx.h"

#include "vla.h"

/*
 * a decl that has a VM type may be referred to more than once,
 * so we need to stash the sizes of its VMs on the stack so we don't
 * evaluate the expressions multiple times, e.g.
 *
 *     int vla[i];
 *     i = 5;
 *     return sizeof vla;
 *
 * or
 *
 *     short vla[f()][g()];
 *     return sizeof(vla) + sizeof(vla[0]);
 *
 * compare this to:
 *
 *     return sizeof(int [f()])
 *
 * there isn't any way to refer the the anonymous type twice,
 * so for this we can just generate the expression directly
 */

unsigned vla_decl_space(decl *d)
{
	const unsigned pws = platform_word_size();
	type *t;
	unsigned sz;

	if(type_is_vla(d->ref, VLA_ANY_DIMENSION))
		sz = pws * 2; /* T *ptr; void *orig_sp; */
	else
		sz = pws; /* T *ptr; - no stack res, no orig_sp */

	for(t = d->ref; t; t = type_next(t))
		if(type_is_vla(t, VLA_TOP_DIMENSION))
			sz += pws;

	return sz;
}

static const out_val *vla_gen_size_ty(
		type *t, out_ctx *octx,
		type *const arith_ty,
		long stack_off, int gen_exprs)
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
				type *ptrsizety = type_ptr_to(arith_ty);
				const out_val *sz;
				const out_val *this_sz;
				long new_stack_off = stack_off == -1
					? -1
					: stack_off - platform_word_size();

				if(gen_exprs){
					this_sz = out_cast(
							octx, gen_expr(t->bits.array.size, octx),
							arith_ty, 0);
				}else{
					assert(stack_off != -1);

					this_sz = out_deref(octx,
							v_new_bp3_below(octx, NULL, ptrsizety, stack_off));
				}

				sz = out_op(
						octx, op_multiply,
						vla_gen_size_ty(
							type_next(t), octx, arith_ty, new_stack_off, gen_exprs),
						this_sz);

				if(stack_off != -1){
					void **pvlamap;
					dynmap *vlamap;

					out_val_retain(octx, sz);
					out_store(octx,
							v_new_bp3_below(octx, NULL, ptrsizety, stack_off),
							sz);

					vlamap = *(pvlamap = out_user_ctx(octx));
					if(!vlamap){
						/* type * => long */
						vlamap = *pvlamap = dynmap_new(NULL, (dynmap_hash_f *)type_hash);
					}

					(void)dynmap_set(type *, long, vlamap, t, stack_off);
				}

				return sz;

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
			return vla_gen_size_ty(type_skip_all(t), octx,
					arith_ty, stack_off, gen_exprs);
	}

	return out_op(octx, op_multiply,
			out_new_l(octx, arith_ty, mul),
			vla_gen_size_ty(type_next(t), octx, arith_ty, stack_off, gen_exprs));
}

static const out_val *vla_gen_size(type *t, out_ctx *octx)
{
	return vla_gen_size_ty(
			t, octx,
			type_nav_btype(cc1_type_nav, type_long),
			-1, 1);
}

void vla_alloc_decl(decl *d, out_ctx *octx)
{
	sym *s = d->sym;
	const out_val *v_sz;
	const out_val *v_ptr;
	type *sizety = type_nav_btype(cc1_type_nav, type_long);
	type *ptrsizety = type_ptr_to(sizety);
	const unsigned pws = platform_word_size();
	const int is_vla = !!type_is_vla(d->ref, VLA_ANY_DIMENSION);
	const unsigned stack_off = d->sym->loc.stack_pos + octx->stack_local_offset;

	assert(s && "no sym for vla");

	if(is_vla){
		/* save the stack pointer */
		out_comment(octx, "save stack for %s", decl_to_str(d));
		out_store(octx,
				v_new_bp3_below(octx, NULL,
					ptrsizety, stack_off - pws),
				v_new_sp3(octx, NULL, sizety, 0));
	}

	out_comment(octx, "gen size for %s", decl_to_str(d));
	v_sz = vla_gen_size_ty(d->ref, octx, sizety,
			/* 2 * platform_word_size - once for vla pointer, once for saved $sp */
			stack_off - (1 + is_vla) * pws,
			1);

	if(is_vla){
		v_sz = out_cast(octx, v_sz, sizety, 0);

		out_comment(octx, "alloca for %s", decl_to_str(d));
		v_ptr = out_alloca_push(octx, v_sz, type_align(d->ref, NULL));

		out_comment(octx, "save ptr for %s", decl_to_str(d));
		out_store(octx,
				v_new_bp3_below(octx, NULL, ptrsizety, stack_off),
				v_ptr);
	}else{
		out_val_consume(octx, v_sz);
	}
}

static const out_val *vla_read(decl *d, out_ctx *octx, long offset)
{
	type *ptr_to_vla_ty = type_ptr_to(type_ptr_to(d->ref));

	return out_deref(octx,
			v_new_bp3_below(octx, NULL, ptr_to_vla_ty,
				octx->stack_local_offset + d->sym->loc.stack_pos - offset));
}

const out_val *vla_address(decl *d, out_ctx *octx)
{
	out_comment(octx, "vla address (%s)", decl_to_str(d));
	return vla_read(d, octx, 0);
}

const out_val *vla_saved_ptr(decl *d, out_ctx *octx)
{
	out_comment(octx, "vla saved stack ptr (%s)", decl_to_str(d));
	return vla_read(d, octx, platform_word_size());
}

const out_val *vla_size(type *const qual_t, out_ctx *octx)
{
	type *t = type_skip_all(qual_t);
	type *ptrsizety = type_ptr_to(type_nav_btype(cc1_type_nav, type_long));
	dynmap *vlamap = *out_user_ctx(octx);

	if(vlamap){
		long stack_off = dynmap_get(type *, long, vlamap, t);

		if(stack_off){
			out_comment(octx, "vla saved size for %s", type_to_str(qual_t));
			return out_deref(octx,
					v_new_bp3_below(octx, NULL, ptrsizety, stack_off));
		}
	}
	/* no cached size */
	out_comment(octx, "vla gen size (%s)", type_to_str(qual_t));
	return vla_gen_size(qual_t, octx);
}
