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

	if((d->store & STORE_MASK_STORE) == store_typedef)
		sz = 0; /* just the sizes */
	else if(type_is_vla(d->ref, VLA_ANY_DIMENSION))
		sz = pws * 2; /* T *ptr; void *orig_sp; */
	else
		sz = pws; /* T *ptr; - no stack res, no orig_sp */

	for(t = d->ref; t; t = type_next(t))
		if(type_is_vla(t, VLA_TOP_DIMENSION))
			sz += pws;

	return sz;
}

static const out_val *vla_cached_size(type *const qual_t, out_ctx *octx)
{
	type *t = type_skip_all(qual_t);
	dynmap *vlamap = *out_user_ctx(octx);

	if(vlamap){
		const out_val *stack_off = dynmap_get(type *, const out_val *, vlamap, t);

		if(stack_off){
			out_comment(octx, "vla saved size for %s", type_to_str(qual_t));

			out_val_retain(octx, stack_off);
			return out_deref(octx, stack_off);
		}
	}

	return NULL;
}

static void vla_cache_size(
		type *const qual_t, out_ctx *octx,
		type *const arith_ty,
		const out_val *sz,
		const out_val *stack_ent)
{
	type *ptrsizety = type_ptr_to(arith_ty);
	void **pvlamap;
	dynmap *vlamap;

	stack_ent = out_change_type(octx, stack_ent, ptrsizety);
	out_val_retain(octx, stack_ent);
	out_store(octx, stack_ent, sz);

	vlamap = *(pvlamap = out_user_ctx(octx));
	if(!vlamap){
		/* type * => out_val const* */
		vlamap = *pvlamap = dynmap_new(type *, NULL, type_hash);
	}

	(void)dynmap_set(type *, const out_val *, vlamap, qual_t, stack_ent);
}

void vla_cleanup(out_ctx *octx)
{
	void **pvlamap = out_user_ctx(octx);
	dynmap *vlamap = *pvlamap;
	size_t i;
	const out_val *v;

	if(!vlamap)
		return;

	for(i = 0; (v = dynmap_value(const out_val *, vlamap, i)); i++)
		out_val_release(octx, v);

	dynmap_free(vlamap);
	*pvlamap = NULL;
}

static const out_val *vla_gen_size_ty(
		type *t, out_ctx *octx,
		type *const arith_ty,
		const out_val *stack_ent)
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
				const out_val *sz;
				const out_val *new_stack_ent = NULL;

				if(stack_ent){
					new_stack_ent = out_op(octx, op_plus,
							stack_ent,
							out_new_l(octx, arith_ty, platform_word_size()));
				}

				sz = vla_cached_size(t, octx);
				if(sz)
					return sz;

				sz = out_op(
						octx, op_multiply,
						vla_gen_size_ty(
							type_next(t), octx, arith_ty, new_stack_ent),
						out_cast(
							octx, gen_expr(t->bits.array.size, octx),
							arith_ty, 0));

				if(stack_ent){
					out_val_retain(octx, sz);
					vla_cache_size(t, octx, arith_ty, sz, stack_ent);
				}

				return sz;

			}else if(t->bits.array.size){
				mul = const_fold_val_i(t->bits.array.size);
			}else{
				/* int (*p)[][E]; // is valid, just can't ptr-arith on it */
				mul = 0;
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
			return vla_gen_size_ty(type_skip_all(t), octx, arith_ty, stack_ent);
	}

	return out_op(octx, op_multiply,
			out_new_l(octx, arith_ty, mul),
			vla_gen_size_ty(type_next(t), octx, arith_ty, stack_ent));
}

static const out_val *vla_gen_size(type *t, out_ctx *octx)
{
	return vla_gen_size_ty(
			t, octx,
			type_nav_btype(cc1_type_nav, type_long),
			NULL);
}

void vla_typedef_init(decl *d, out_ctx *octx)
{
	type *sizety = type_nav_btype(cc1_type_nav, type_long);
	const out_val *alloc_start = d->sym->outval;

	out_val_retain(octx, alloc_start);
	out_val_consume(octx,
			vla_gen_size_ty(
				d->ref, octx, sizety,
				alloc_start));
}

void vla_decl_init(decl *d, out_ctx *octx)
{
	sym *s = d->sym;
	const out_val *v_sz;
	const out_val *v_ptr;
	type *sizety = type_nav_btype(cc1_type_nav, type_long);
	const unsigned pws = platform_word_size();
	const int is_vla = !!type_is_vla(d->ref, VLA_ANY_DIMENSION);
	const out_val *stack_ent;

	stack_ent = d->sym->outval;

	assert(s && "no sym for vla");

	if(is_vla){
		const out_val *vla_saved_sp;

		/* save the stack pointer */
		out_comment(octx, "save stack for %s", decl_to_str(d));

		out_val_retain(octx, stack_ent);
		vla_saved_sp = out_op(octx, op_plus,
				stack_ent, out_new_l(octx, sizety, pws));

		out_store(octx, vla_saved_sp,
				v_new_sp3(octx, NULL, sizety, 0));
	}

	out_comment(octx, "gen size for %s", decl_to_str(d));
	{
		const out_val *generated_sz_loc;

		/* 2 * platform_word_size - once for vla pointer, once for saved $sp */
		out_val_retain(octx, stack_ent);
		generated_sz_loc = out_op(octx, op_plus,
				stack_ent, out_new_l(octx, sizety, (1 + is_vla) * pws));

		v_sz = vla_gen_size_ty(d->ref, octx, sizety, generated_sz_loc);
	}

	if(is_vla){
		v_sz = out_cast(octx, v_sz, sizety, 0);

		out_comment(octx, "alloca for %s", decl_to_str(d));
		v_ptr = out_alloca_push(octx, v_sz, type_align(d->ref, NULL));

		out_comment(octx, "save ptr for %s", decl_to_str(d));
		out_val_retain(octx, stack_ent);
		out_store(octx, stack_ent, v_ptr);
	}else{
		out_val_consume(octx, v_sz);
	}
}

static const out_val *vla_read(decl *d, out_ctx *octx, long offset)
{
	type *sizety = type_nav_btype(cc1_type_nav, type_long);
	const out_val *stack_ent = d->sym->outval;

	out_val_retain(octx, stack_ent);

	stack_ent = out_op(octx, op_plus,
			stack_ent, out_new_l(octx, sizety, offset));

	return out_deref(octx, stack_ent);
}

const out_val *vla_address(decl *d, out_ctx *octx)
{
	out_comment(octx, "vla address (%s)", decl_to_str(d));
	return vla_read(d, octx, 0);
}

const out_val *vla_saved_ptr(decl *d, out_ctx *octx)
{
	out_comment(octx, "vla saved stack ptr (%s)", decl_to_str(d));
	return out_change_type(octx,
			vla_read(d, octx, platform_word_size()),
			type_ptr_to(type_nav_btype(cc1_type_nav, type_void)));
}

const out_val *vla_size(type *const qual_t, out_ctx *octx)
{
	const out_val *cached_sz = vla_cached_size(qual_t, octx);

	if(cached_sz)
		return cached_sz;

	out_comment(octx, "vla gen size (%s)", type_to_str(qual_t));
	return vla_gen_size(qual_t, octx);
}
