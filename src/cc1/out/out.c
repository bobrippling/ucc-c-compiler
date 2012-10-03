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

/*
 * This entire stack-output idea was inspired by tinycc, and improved somewhat
 */

#define N_VSTACK 1024
struct vstack vstack[N_VSTACK];
struct vstack *vtop = NULL;

static int reserved_regs[N_REGS];

void vpush(decl *d)
{
	if(!vtop){
		vtop = vstack;
	}else{
		UCC_ASSERT(vtop < vstack + N_VSTACK - 1,
				"vstack overflow, vtop=%p, vstack=%p, diff %ld",
				(void *)vtop, (void *)vstack, vtop - vstack);

		if(vtop->type == FLAG)
			v_to_reg(vtop);

		vtop++;
	}

	vtop_clear(d);
}

void v_clear(struct vstack *vp, decl *d)
{
	memset(vp, 0, sizeof *vp);
	vp->d = d;
}

void vtop_clear(decl *d)
{
	v_clear(vtop, d);
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

void out_flush_volatile(void)
{
	if(vtop)
		v_to_reg(vtop);
}

void out_assert_vtop_null(void)
{
	UCC_ASSERT(!vtop, "vtop not null");
}

void out_dump(void)
{
	int i;

	for(i = 0; &vstack[i] <= vtop; i++)
		fprintf(stderr, "vstack[%d] = { %d, %d }\n",
				i, vstack[i].type, vstack[i].bits.reg);
}

void vswap(void)
{
	struct vstack tmp;
	memcpy(&tmp, vtop, sizeof tmp);
	memcpy(vtop, &vtop[-1], sizeof tmp);
	memcpy(&vtop[-1], &tmp, sizeof tmp);
}

void out_swap(void)
{
	vswap();
}

void v_prepare_op(struct vstack *vp)
{
	switch(vp->type){
		case STACK:
		case LBL:
			/* need to pull the values from the stack */

		case STACK_SAVE:
			/* ditto, impl handles pulling from stack */

		case FLAG:
			/* obviously can't have a flag in cmp/mov code */
			v_to_reg(vp);

		case REG:
		case CONST:
			break;
	}
}

void vtop2_prepare_op(void)
{
#if 0
	/* this assume we're assigning back */
	/* special case - const and stack/lbl is fine */
	if((vtop->type == CONST && vtop[-1].type == STACK)
	|| (vtop->type == STACK && vtop[-1].type == CONST))
	{
		return;
	}
#endif


	/* attempt to give this a higher reg,
	 * since it'll be used more,
	 * e.g. return and idiv */
	v_prepare_op(&vtop[-1]);

	v_prepare_op(vtop);

	if((vtop->type == vtop[-1].type && vtop->type != REG) || vtop[-1].type != REG){
		/* prefer putting vtop[-1] in a reg, since that's returned after */
		v_to_reg(&vtop[-1]);
	}
}

int v_unused_reg(int stack_as_backup)
{
	struct vstack *it, *first;
	int used[N_REGS];
	int i;

	memcpy(used, reserved_regs, sizeof used);
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
		int reg = first->bits.reg;

		v_freeup_regp(first);

		return reg;
	}
	return -1;
}

int v_to_reg(struct vstack *conv)
{
	if(conv->type != REG)
		impl_load(conv, v_unused_reg(1));

	return conv->bits.reg;
}

void v_to_mem(struct vstack *conv)
{
	switch(conv->type){
		case CONST:
		case FLAG:
		case STACK_SAVE: /* pull from stack, save to stack,
												-O3, replace current stack */
			v_to_reg(conv);

		case REG:
			v_save_reg(conv);

		case STACK:
		case LBL:
			break;
	}
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

void v_freeup_regp(struct vstack *vp)
{
	/* freeup this reg */
	int r;

	UCC_ASSERT(vp->type == REG, "not reg");

	/* attempt to save to a register first */
	r = v_unused_reg(0);

	if(r >= 0){
		impl_reg_cp(vp, r);

		v_clear(vp, NULL);
		vp->type = REG;
		vp->bits.reg = r;

	}else{
		v_save_reg(vp);
	}
}

void v_save_reg(struct vstack *vp)
{
	struct vstack store;

	UCC_ASSERT(vp->type == REG, "not reg");

	memset(&store, 0, sizeof store);

	store.type = STACK;
	store.d = decl_ptr_depth_inc(decl_copy(vp->d));
	store.bits.off_from_bp = -impl_alloc_stack(decl_size(store.d));

	impl_store(vp, &store);

	store.type = STACK_SAVE;

	memcpy(vp, &store, sizeof store);
}

void v_freeup_reg(int r, int allowable_stack)
{
	struct vstack *vp = v_find_reg(r);

	if(vp && vp < &vtop[-allowable_stack + 1])
		v_freeup_regp(vp);
}

void v_freeup_regs(int a, int b)
{
	reserved_regs[a] = 1;
	reserved_regs[b] = 1;

	v_freeup_reg(a, 2);
	v_freeup_reg(b, 2);

	reserved_regs[a] = 0;
	reserved_regs[b] = 0;
}

void v_inv_cmp(struct vstack *vp)
{
	switch(vp->bits.flag){
#define OPPOSITE(from, to) case flag_ ## from: vp->bits.flag = flag_ ## to; return
		OPPOSITE(eq, ne);
		OPPOSITE(ne, eq);

		OPPOSITE(le, gt);
		OPPOSITE(gt, le);

		OPPOSITE(lt, ge);
		OPPOSITE(ge, lt);

		/*OPPOSITE(z, nz);
		OPPOSITE(nz, z);*/
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
	vpush(d);

	vtop->type = CONST;
	vtop->bits.val = iv->val; /* TODO: unsigned */
}

void out_push_i(decl *d, int i)
{
	intval iv = {
		.val = i,
		.suffix = 0
	};

	out_push_iv(d, &iv);
}

void out_push_lbl(char *s, int pic, decl *d)
{
	vpush(d);

	vtop->bits.lbl.str = s;
	vtop->bits.lbl.pic = pic;

	vtop->type = LBL;
}

void vdup(void)
{
	vpush(NULL);
	memcpy(&vtop[0], &vtop[-1], sizeof *vtop);
}

void out_dup(void)
{
	vdup();
}

void out_store()
{
	struct vstack *store, *val;

	val   = &vtop[0];
	store = &vtop[-1];

	impl_store(val, store);

	/* swap, popping the store, but not the value */
	vswap();
	vpop();
}

void out_normalise(void)
{
	if(vtop->type == CONST)
		vtop->bits.val = !!vtop->bits.val;
	else
		impl_normalise();
}

void out_push_sym(sym *s)
{
	vpush(decl_ptr_depth_inc(decl_copy(s->decl)));

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
}

void out_push_sym_val(sym *s)
{
	out_push_sym(s);
	out_deref();
}

static void vtop2_are(
		enum vstore a,
		enum vstore b,
		struct vstack **pa, struct vstack **pb)
{
	if(vtop->type == a)
		*pa = vtop;
	else if(vtop[-1].type == a)
		*pa = &vtop[-1];
	else
		*pa = NULL;

	if(vtop->type == b)
		*pb = vtop;
	else if(vtop[-1].type == b)
		*pb = &vtop[-1];
	else
		*pb = NULL;
}

static int calc_ptr_step(decl *d)
{
	int sz;

	if(!d)
		return 1; /* void * */

	if(decl_ptr_depth(d) > 1)
		return decl_size(d);

	sz = type_size(d->type);

	/* array? if so, sizeof the array */
	fprintf(stderr, "calc_ptr_step(%s)\n", decl_to_str(d));
	if(decl_is_array(d))
		sz *= decl_inner_array_count(d);

	return sz;
}

void out_op(enum op_type op)
{
	/*
	 * the implementation does a vpop() and
	 * sets vtop sufficiently to describe how
	 * the result is returned
	 */

	struct vstack *t_const, *t_stack;

	/* check for adding or subtracting to stack */
	vtop2_are(CONST, STACK, &t_const, &t_stack);

	if(t_const && t_stack){
		/* t_const == vtop... should be */
		t_stack->bits.off_from_bp += t_const->bits.val * calc_ptr_step(t_stack->d);

		goto fin;

	}else if(t_const){
		/* TODO: -O1, constant folding here */

		switch(op){
			case op_plus:
			case op_minus:
				if(t_const->bits.val == 0)
					goto fin;
			default:
				break;

			case op_multiply:
			case op_divide:
				if(t_const->bits.val == 1)
					goto fin;
		}

		goto def;
fin:
		vpop();

	}else{
		int div;

def:
		div = 0;

		switch(op){
			case op_plus:
			case op_minus:
			{
				int l_ptr, r_ptr;

				l_ptr = !vtop->d    || decl_is_ptr(vtop->d);
				r_ptr = !vtop[-1].d || decl_is_ptr(vtop[-1].d);

				if(l_ptr || r_ptr){
					const int ptr_step = calc_ptr_step(l_ptr ? vtop->d : vtop[-1].d);

					if(l_ptr ^ r_ptr){
						/* ptr +/- int, adjust the non-ptr by sizeof *ptr */
						struct vstack *val = &vtop[l_ptr ? -1 : 0];

						switch(val->type){
							case CONST:
								val->bits.val *= ptr_step;
								break;

							default:
								v_to_reg(val);

							case REG:
							{
								int swap;
								if((swap = (val != vtop)))
									vswap();

								out_push_i(NULL, ptr_step);
								out_op(op_multiply);

								if(swap)
									vswap();
							}
						}

					}else if(l_ptr && r_ptr){
						/* difference - divide afterwards */
						div = ptr_step;
					}
				}
				break;
			}

			default:
				break;
		}

		impl_op(op);

		if(div){
			out_push_i(NULL, div);
			out_op(op_divide);
		}
	}
}

void v_deref_decl(struct vstack *vp)
{
	/* XXX: memleak */
	vp->d = decl_ptr_depth_dec(decl_copy(vp->d), NULL);
}

void out_deref()
{
	decl *indir;
	/*
	 >   *((int (*)[])exp
	 is a no-op
	 i.e. if the pointed-to object is array-type, don't deref
	 */

	out_comment("deref %s", decl_to_str(vtop->d));

	indir = decl_ptr_depth_dec(decl_copy(vtop->d), NULL);

	if(decl_is_array(indir) || decl_is_fptr(vtop->d)){
		out_change_decl(indir);
		return; /* noop */
	}

	switch(vtop->type){
		case FLAG:
			ICE("deref of flag");

		case CONST:
		case REG:
		case STACK:
		case LBL:
		case STACK_SAVE:
			impl_deref();
			break;
	}
}


void out_op_unary(enum op_type op)
{
	switch(op){
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
				case STACK_SAVE:
				case LBL:
					break;
			}
	}

	impl_op_unary(op);
}

void out_cast(decl *from, decl *to)
{
	/* casting vtop - don't bother if it's a constant, just change the size */
	if(vtop->type != CONST)
		impl_cast(from, to);

	out_change_decl(to);
}

void out_change_decl(decl *d)
{
	/* XXX: memleak */
	vtop->d = d;
}

void out_call(int nargs, decl *rt, decl *call)
{
	impl_call(nargs, rt, call);
}

void out_jmp(void)
{
	if(vtop > vstack){
		/* flush the stack-val we need to generate before the jump */
		vtop--;
		out_flush_volatile();
		vtop++;
	}

	impl_jmp();

	vpop();
}

void out_jtrue(const char *lbl)
{
	impl_jcond(1, lbl);

	vpop();
}

void out_jfalse(const char *lbl)
{
	int cond = 0;

	if(vtop->type == FLAG){
		v_inv_cmp(vtop);
		cond = 1;
	}

	impl_jcond(cond, lbl);

	vpop();
}

void out_label(const char *lbl)
{
	/* if we have volatile data, ensure it's in a register */
	out_flush_volatile();

	impl_lbl(lbl);
}

void out_comment(const char *fmt, ...)
{
	va_list l;

	va_start(l, fmt);
	impl_comment(fmt, l);
	va_end(l);
}

void out_func_prologue(int stack_res, int nargs, int variadic)
{
	impl_func_prologue(stack_res, nargs, variadic);
}

void out_func_epilogue()
{
	impl_func_epilogue();
}

void out_pop_func_ret(decl *d)
{
	impl_pop_func_ret(d);
}

void out_undefined(void)
{
	out_flush_volatile();
	impl_undefined();
}

void out_push_frame_ptr(int nframes)
{
	out_flush_volatile();

	vpush(NULL);
	vtop->type = REG;
	vtop->bits.reg = impl_frame_ptr_to_reg(nframes);
}

int out_n_call_regs(void)
{
	return N_CALL_REGS;
}
