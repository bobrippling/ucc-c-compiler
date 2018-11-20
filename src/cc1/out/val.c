#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../../util/alloc.h"

#include "../type.h"
#include "../type_nav.h"

#include "../macros.h"

#include "val.h"
#include "ctx.h"
#include "virt.h"

#include "../op.h"
#include "asm.h"
#include "impl.h"
#include "out.h" /* retain/release prototypes */

#include "../cc1.h" /* cc1_type_nav */

const char *v_store_to_str(enum out_val_store store)
{
	switch(store){
		CASE_STR(V_CONST_I);
		CASE_STR(V_REG);
		CASE_STR(V_REG_SPILT);
		CASE_STR(V_LBL);
		CASE_STR(V_CONST_F);
		CASE_STR(V_FLAG);
	}
	return NULL;
}

static void v_register(out_ctx *octx, out_val_list *l)
{
	if(octx->val_head)
		assert(!octx->val_head->prev);

	/* double link */
	l->next = octx->val_head;
	if(octx->val_head)
		octx->val_head->prev = l;

	/* store in val_head */
	octx->val_head = l;

	if(!octx->val_tail)
		octx->val_tail = l->next;
}

static void v_init(out_val *v, type *ty)
{
	v->retains = 1;
	v->t = ty;
}

out_val *v_new(out_ctx *octx, type *ty)
{
	out_val_list *l = umalloc(sizeof *l);
	out_val *v = &l->val;

	v_init(v, ty);
	v_register(octx, l);

	return v;
}

static void v_decay_flag(out_ctx *octx, const out_val *flag)
{
	const out_val *regged = v_to_reg(octx, flag);

	if(regged == flag)
		return;

	out_val_overwrite((out_val *)flag, regged);
	out_val_release(octx, regged);
	out_val_retain(octx, flag);
}

void v_decay_flags_except(out_ctx *octx, const out_val *except[])
{
	if(!octx->check_flags)
		return;
	octx->check_flags = 0;
	{
		out_val_list *iter;

		for(iter = octx->val_head; iter; iter = iter->next){
			out_val *v = &iter->val;

			if(v->retains > 0 && v->type == V_FLAG){
				const out_val **vi;
				int found = 0;

				assert(!v->phiblock && "phis should never be created as flags");

				for(vi = except; vi && *vi; vi++){
					if(v == *vi){
						found = 1;
						break;
					}
				}

				if(!found){
					out_comment(octx, "saving flag");
					v_decay_flag(octx, v);
				}
			}
		}
	}
	octx->check_flags = 1;
}

void v_decay_flags_except1(out_ctx *octx, const out_val *except)
{
	const out_val *ar[] = { except, NULL };
	v_decay_flags_except(octx, ar);
}

void v_decay_flags(out_ctx *octx)
{
	v_decay_flags_except(octx, NULL);
}

static out_val *v_dup(out_ctx *octx, const out_val *from, type *ty)
{
	switch(from->type){
		case V_CONST_I:
		case V_CONST_F:
		case V_LBL:
copy:
		{
			out_val *v = v_new(octx, ty);
			out_val_overwrite(v, from);
			v->t = ty;
			return v;
		}

		case V_FLAG:
		{
			const out_val *dup;

			out_val_retain(octx, from);
			dup = v_to_reg(octx, from);

			out_val_overwrite((out_val *)from, dup); /* replace flag */
			out_val_release(octx, dup);

			/* fall */
		}

		case V_REG_SPILT:
		case V_REG:
		{
			struct vreg r;
			out_val *new;

			/* if it's a frame constant we can just use the same register */
			if(impl_reg_frame_const(&from->bits.regoff.reg, 0))
				goto copy;

			/* copy to a new register */
			v_unused_reg(
						octx, /*stack backup:*/1,
						from->bits.regoff.reg.is_float, &r,
						NULL);

			/* dup */
			impl_reg_cp_no_off(octx, from, &r);

			new = v_new(octx, ty);
			new->type = from->type;
			new->bits.regoff.reg = r;
			new->bits.regoff.offset = from->bits.regoff.offset;

			return new;
		}
	}

	assert(0);
}

static out_val *v_reuse(out_ctx *octx, const out_val *from, type *ty)
{
	out_val *mut;

	if(!from || from->retains > 1){
		if(from)
			out_val_consume(octx, from);

		return v_new(octx, ty);
	}

	assert(from->retains == 1);
	mut = (out_val *)from;

	mut->t = ty;
	return mut; /* reuse */
}

out_val *v_dup_or_reuse(out_ctx *octx, const out_val *from, type *ty)
{
	assert(from);

	if(from->retains > 1){
		out_val *r = v_dup(octx, from, ty);
		out_val_consume(octx, from);
		return r;
	}

	return v_reuse(octx, from, ty);
}

out_val *v_new_flag(
		out_ctx *octx, const out_val *from,
		enum flag_cmp cmp, enum flag_mod mod)
{
	out_val *v = v_reuse(octx, from,
			type_nav_btype(cc1_type_nav, type__Bool));

	v->type = V_FLAG;
	v->bits.flag.cmp = cmp;
	v->bits.flag.mods = mod;
	return v;
}

out_val *v_new_reg(
		out_ctx *octx, const out_val *from,
		type *ty, const struct vreg *reg)
{
	/* reg may alias from->bits... */
	const struct vreg savedreg = *reg;
	out_val *v = v_reuse(octx, from, ty);
	v->type = V_REG;
	v->bits.regoff.offset = 0;
	memcpy_safe(&v->bits.regoff.reg, &savedreg);
	return v;
}

out_val *v_new_sp(out_ctx *octx, const out_val *from)
{
	struct vreg r;

	r.is_float = 0;
	r.idx = REG_SP;

	octx->used_stack = 1;

	return v_new_reg(octx, from, type_nav_voidptr(cc1_type_nav), &r);
}

out_val *v_new_sp3(
		out_ctx *octx, const out_val *from,
		type *ty, long stack_pos)
{
	out_val *v = v_new_sp(octx, from);
	v->t = ty;
	v->bits.regoff.offset = stack_pos;
	return v;
}

static out_val *v_new_bp3(
		out_ctx *octx, const out_val *from,
		type *ty, long stack_pos)
{
	out_val *v = v_new_sp3(octx, from, ty, stack_pos);
	v->bits.regoff.reg.idx = REG_BP;
	return v;
}

out_val *v_new_bp3_above(
		out_ctx *octx, const out_val *from,
		type *ty, long stack_pos)
{
	return v_new_bp3(octx, from, ty, stack_pos);
}

out_val *v_new_bp3_below(
		out_ctx *octx, const out_val *from,
		type *ty, long stack_pos)
{
	return v_new_bp3(octx, from, ty, -stack_pos);
}

void v_try_stack_reclaim(out_ctx *octx)
{
	/* if we have no out_vals on the stack,
	 * we can reclaim stack-spill space.
	 * this is a simple algorithm for reclaiming */
	out_val_list *iter;
	long lowest = 0;

	if(octx->in_prologue || octx->alloca_count)
		return;

	/* only reclaim if we have an empty val list */
	for(iter = octx->val_head; iter; iter = iter->next){
		if(iter->val.retains == 0)
			continue;
		if(iter->val.phiblock)
			continue;
		switch(iter->val.type){
			case V_REG:
			case V_REG_SPILT:
				if(!impl_reg_frame_const(&iter->val.bits.regoff.reg, 0))
					return;
				if(iter->val.bits.regoff.offset < lowest)
					lowest = iter->val.bits.regoff.offset;
				break;
			case V_CONST_I:
			case V_LBL:
			case V_CONST_F:
			case V_FLAG:
				break;
		}
	}

	lowest = -lowest;

	v_stackt reclaim = octx->cur_stack_sz - lowest;
	if(reclaim > 0){
		if(fopt_mode & FOPT_VERBOSE_ASM)
			out_comment(octx, "reclaim %ld (%ld start %ld lowest)",
					reclaim, octx->initial_stack_sz, lowest);

		octx->cur_stack_sz = lowest;
	}
}

const out_val *out_val_release(out_ctx *octx, const out_val *v)
{
	out_val *mut = (out_val *)v;
	assert(v && "release NULL out_val");
	assert(mut->retains > 0 && "double release");
	if(--mut->retains == 0){
		v_try_stack_reclaim(octx);

		return NULL;
	}
	return mut;
}

const out_val *out_val_retain(out_ctx *octx, const out_val *v)
{
	(void)octx;
	assert(v->retains > 0);
	((out_val *)v)->retains++;
	return v;
}

void out_val_overwrite(out_val *d, const out_val *s)
{
	/* don't copy .retains */
	d->type = s->type;
	d->t = s->t;
	d->bitfield = s->bitfield;
	d->bits = s->bits;
}

const out_val *out_annotate_likely(
		out_ctx *octx, const out_val *val, int unlikely)
{
	out_val *mut = v_dup_or_reuse(octx, val, val->t);

	mut->flags |= unlikely ? VAL_FLAG_UNLIKELY : VAL_FLAG_LIKELY;

	return mut;
}

int vreg_eq(const struct vreg *a, const struct vreg *b)
{
	return a->idx == b->idx && a->is_float == b->is_float;
}

const char *out_get_lbl(const out_val *v)
{
	return v->type == V_LBL ? v->bits.lbl.str : NULL;
}

int out_is_nonconst_temporary(const out_val *v)
{
	switch(v->type){
		case V_CONST_I:
		case V_CONST_F:
		case V_LBL:
			break;
		case V_REG:
		case V_REG_SPILT:
		case V_FLAG:
			return 1;
	}
	return 0;
}
