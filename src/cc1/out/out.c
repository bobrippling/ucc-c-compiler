#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../data_structs.h"
#include "out.h"
#include "vstack.h"
#include "x86_64.h"
#include "../../util/platform.h"
#include "../cc1.h"

#define TODO() fprintf(cc1_out, "// TODO: %s\n", __func__)

/*
 * This entire stack-output idea was inspired by tinycc, and improved slightly,
 */

#define N_VSTACK 1024
struct vstack vstack[N_VSTACK];
struct vstack *vtop = NULL;

void vpush(void)
{
	if(!vtop){
		vtop = vstack;
	}else{
		UCC_ASSERT(vtop < vstack + N_VSTACK - 1,
				"vstack overflow, vtop=%p, vstack=%p, diff %ld",
				(void *)vtop, (void *)vstack, vtop - vstack);

		if(vtop->type == FLAG)
			v_save_reg(vtop);

		vtop++;
	}
	vtop_clear();
}

void v_clear(struct vstack *vp)
{
	memset(vp, 0, sizeof *vp);
}

void vtop_clear(void)
{
	v_clear(vtop);
}

void vpop(void)
{
	UCC_ASSERT(vtop, "NULL vtop for vpop");

	if(vtop == vstack){
		vtop = NULL;
	}else{
		UCC_ASSERT(vtop < vstack + N_VSTACK - 1, "vstack underflow");
		vtop--;
	}
}

void out_assert_vtop_null(void)
{
	UCC_ASSERT(!vtop, "vtop not null");
}

void vswap(void)
{
	struct vstack tmp;
	memcpy(&tmp, vtop, sizeof tmp);
	memcpy(vtop, &vtop[-1], sizeof tmp);
	memcpy(&vtop[-1], &tmp, sizeof tmp);
}

void vtop2_prepare_op(void)
{
	/* both can't be const, memory, or flags */
	if(vtop->type == vtop[-1].type && vtop->type != REG){
		/* prefer putting vtop[-1] in a reg, since that's returned after */
		v_to_reg(&vtop[-1]);
	}
}

int v_unused_reg(int stack_as_backup)
{
	struct vstack *it, *first;
	int used[N_REGS];
	int i;

	memset(used, 0, sizeof used);
	first = NULL;

	for(it = vstack; it <= vtop; it++){
		if(it->type == REG){
			if(!first)
				first = it;
			used[it->bits.reg] = 1;
		}
	}

	for(i = 0; i < N_REGS; i++)
		if(!used[i])
			return i;

	if(stack_as_backup){
		/* no free regs, move `first` to the stack and claim its reg */
		v_save_reg(first);

		return first->bits.reg;
	}
	return -1;
}

int v_to_reg(struct vstack *conv)
{
	if(conv->type != REG){
		int reg = v_unused_reg(1);

		impl_load(conv, reg);

		v_clear(conv);
		conv->type = REG;
		conv->bits.reg = reg;
	}

	return conv->bits.reg;
}

struct vstack *v_find_reg(int reg)
{
	struct vstack *vp;
	if(!vtop)
		return NULL;

	for(vp = vstack; vp <= vtop; vp++)
		if(vp->type == REG && vp->bits.reg == reg)
			return vp;

	return NULL;
}

void v_save_reg(struct vstack *vp)
{
	switch(vp->type){
		case CONST:
		case STACK:
		case LBL:
			break;

		case FLAG:
			v_to_reg(vp);
			/* fall */

		case REG:
		{
			struct vstack store;
			int r;

			/* attempt to save to a register first */
			r = v_unused_reg(0);

			if(r >= 0){
				store.type = REG;
				store.d = vp->d;
				store.bits.reg = r;
			}else{
				store.type = STACK;
				store.d = vp->d;
				store.bits.off_from_bp = 23; /* TODO */
				ICW("TODO: stack offset");
			}

			impl_store(vp->bits.reg, &store);

			memcpy(vp, &store, sizeof store);
		}
	}
}

void v_freeup_reg(int r, int allowable_stack)
{
	struct vstack *vp = v_find_reg(r);

	if(vp && vp < &vtop[-allowable_stack + 1])
		v_save_reg(vp);
}

static void v_inv_cmp(struct vstack *vp)
{
	switch(vp->bits.flag){
#define OPPOSITE(from, to) case flag_ ## from: vp->bits.flag = flag_ ## to; return
		OPPOSITE(eq, ne);
		OPPOSITE(ne, eq);

		OPPOSITE(le, gt);
		OPPOSITE(gt, le);

		OPPOSITE(lt, ge);
		OPPOSITE(ge, lt);

		OPPOSITE(z, nz);
		OPPOSITE(nz, z);
#undef OPPOSITE
	}
	ICE("invalid op");
}

void out_pop(void)
{
	vpop();
}

void out_push_iv(decl *d, intval *iv)
{
	vpush();
	vtop->type = CONST;
	vtop->bits.val = iv->val; /* TODO: unsigned */
	vtop->d = d;
}

void out_push_i(decl *d, int i)
{
	intval iv = {
		.val = i,
		.suffix = 0
	};

	out_push_iv(d, &iv);
}

void out_push_lbl(char *s, int pic)
{
	vpush();
	vtop->type = LBL;
	vtop->bits.lbl.str = s;
	vtop->bits.lbl.pic = pic;
	vtop->is_addr = 1;
}

void out_dup(void)
{
	vpush();
	memcpy(&vtop[0], &vtop[-1], sizeof *vtop);
}

void out_store()
{
	struct vstack *store, *val;

	val   = &vtop[0];
	store = &vtop[-1];

	/*
	 * TODO: don't force val to be in a reg
	 * currently i = 5:
	 *   mov $5, eax
	 *   mov eax, 0x8(%rbp)
	 * could do:
	 *   mov $5, 0x8(%rbp)
	 */

	impl_store(v_to_reg(val), store);

	/* pop the store, but not the value */
	vpop();
}

void out_normalise(void)
{
	TODO();
}

void out_push_sym_addr(sym *s)
{
	out_push_sym(s);
	vtop->is_addr = 1;
}

void out_push_sym(sym *s)
{
	vpush();

	switch(s->type){
		case sym_local:
			vtop->type = STACK;
			vtop->bits.off_from_bp = -s->offset - platform_word_size();
			break;

		case sym_arg:
			vtop->type = STACK;
			/*
			 * if it's less than N_CALL_ARGS, it's below rbp, otherwise it's above
			 */
			vtop->bits.off_from_bp = (s->offset < N_CALL_REGS
				? -(s->offset + 1)
				:   s->offset - N_CALL_REGS + 2)
				* platform_word_size();
			break;

		case sym_global:
			vtop->type = LBL;
			vtop->bits.lbl.str = s->decl->spel;
			vtop->bits.lbl.pic = 1;
			break;
	}

	vtop->d = s->decl;
}

void out_op(enum op_type op)
{
	/*
	 * the implementation does a vpop() and
	 * sets vtop sufficiently to describe how
	 * the result is returned
	 */
	impl_op(op);
}

void out_op_unary(enum op_type op)
{
	switch(op){
		case op_deref:
			if(vtop->is_addr){
				vtop->is_addr = 0;
				return;
			}
			break;

		case op_plus:
			return;

		default:
			/* special case - reverse the flag if possible */
			switch(vtop->type){
				case FLAG:
					if(op == op_not){
						v_inv_cmp(vtop);
						return;
					}
					break;

				case CONST:
					switch(op){
#define OP(t, o) case op_ ## t: vtop->bits.val = o vtop->bits.val; return
						OP(not, !);
						OP(minus, -);
						OP(bnot, ~);
#undef OP

						default:
							ICE("invalid unary op");
					}
					break;

				case REG:
				case STACK:
				case LBL:
					break;
			}
	}

	impl_op_unary(op);
}

void out_cast(decl *from, decl *to)
{
	(void)to;
	(void)from;
	TODO();
}

void out_call(int nargs)
{
	impl_call(nargs);
}

void out_jmp(void)
{
	impl_jmp();

	vpop();
}

void out_jtrue(const char *lbl)
{
	impl_jtrue(lbl);
}

void out_jfalse(const char *lbl)
{
	out_op_unary(op_not);
	out_jtrue(lbl);
}

void out_label(const char *lbl)
{
	fprintf(cc1_out, "%s:\n", lbl);
}

void out_comment(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	impl_comment(fmt, l);
	va_end(l);
}
