#include "../../util/where.h"
#include "../data_structs.h"
#include "../type_ref.h"

#include "vstack.h"
#include "basic_block.h"

struct basic_blk
{
	enum bb_type
	{
		bb_norm, bb_split, bb_phi
	} type;

	const char **insns;
	struct basic_blk *next;
};

struct basic_blk_split
{
	enum bb_type type;
	struct basic_blk *cond, *btrue, *bfalse;
};

struct basic_blk_phi
{
	enum bb_type type;
	struct basic_blk **froms;
	struct basic_blk *next;
};

basic_blk *bb_new(void)
{
	basic_blk *bb = umalloc(sizeof *bb);
	bb->type = bb_norm;
	return bb;
}

basic_blk *bb_split(
		basic_blk *exp,
		basic_blk *b_true,
		basic_blk *b_false)
{
	basic_blk_split *bb_sp = umalloc(sizeof *bb_sp);

	bb_sp->type = bb_split;
	bb_sp->cond = exp;
	bb_sp->btrue = b_true;
	bb_sp->bfalse = b_false;

	return bb_sp;
}

basic_blk *bb_add_merge(basic_blk *to, basic_blk *from)
{
	basic_blk_phi *phi;
	if(to){
		assert(to->type == bb_merge);
		phi = (basic_blk_phi *)to;
	}else{
		phi = umalloc(sizeof *phi);
		phi->type = basic_blk_phi;
		to = (basic_blk *)phi;
	}

	dynarray_add(&phi->froms, from);

	return to;
}

void out_jmp(basic_blk *b_from, void)
{
	if(vtop > vstack){
		/* flush the stack-val we need to generate before the jump */
		v_flush_volatile(vtop - 1);
	}

	impl_jmp();
	vpop();
}

void out_jtrue(basic_blk *b_from, const char *lbl)
{
	impl_cond(1, lbl);

	vpop();
}

void out_jfalse(basic_blk *b_from, const char *lbl)
{
	int cond = 0;

	if(vtop->type == V_FLAG){
		v_inv_cmp(&vtop->bits.flag);
		cond = 1;
	}

	impl_jcond(cond, lbl);

	vpop();
}

void out_phi_pop_to(basic_blk *b_from, void *vvphi)
{
	struct vstack *const vphi = vvphi;

	/* put the current value into the phi-save area */
	memcpy_safe(vphi, vtop);

	if(vphi->type == V_REG)
		v_reserve_reg(&vphi->bits.regoff.reg); /* XXX: watch me */

	out_pop(b_from);
}

void out_phi_join(basic_blk *b_from, void *vvphi)
{
	struct vstack *const vphi = vvphi;

	if(vphi->type == V_REG)
		v_unreserve_reg(&vphi->bits.regoff.reg); /* XXX: voila */

	/* join vtop and the current phi-save area */
	v_to_reg(vtop);
	v_to_reg(vphi);

	if(!vreg_eq(&vtop->bits.regoff.reg, &vphi->bits.regoff.reg)){
		/* _must_ match vphi, since it's already been generated */
		impl_reg_cp(vtop, &vphi->bits.regoff.reg);
		memcpy_safe(vtop, vphi);
	}
}
