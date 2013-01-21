#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../../util/platform.h"
#include "../data_structs.h"
#include "vstack.h"
#include "impl.h"
#include "../cc1.h"
#include "asm.h"
#include "common.h"
#include "out.h"
#include "lbl.h"

#ifndef MIN
#  define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define REG_STR_SZ 8

#define REG_A 0
#define REG_B 1
#define REG_C 2
#define REG_D 3

#define VSTACK_STR_SZ 128

static const char regs[] = "abcd";
static const struct
{
	char reg, suffix;
	int idx;
} call_regs[] = {
	{ 'd', 'i',  -1 },
	{ 's', 'i',  -1 },
	{ 'd', 'x',  REG_D },
	{ 'c', 'x',  REG_C },
	{ '8', '\0', -1 },
	{ '9', '\0', -1 },
};


static int stack_sz;

static const char *x86_reg_str_r(char buf[REG_STR_SZ], int reg, type_ref *r)
{
	const char *regpre, *regpost;

	UCC_ASSERT(reg < N_REGS, "invalid x86 reg %d", reg);

	asm_reg_name(r, &regpre, &regpost);

	snprintf(buf, REG_STR_SZ, "%s%c%s", regpre, regs[reg], regpost);

	return buf;
}

static const char *reg_str_r(char buf[REG_STR_SZ], struct vstack *reg)
{
	UCC_ASSERT(reg->type == REG, "non-reg %d", reg->type);
	return x86_reg_str_r(buf, reg->bits.reg, reg->t);
}

static const char *vstack_str_r_ptr(char buf[VSTACK_STR_SZ], struct vstack *vs, int ptr)
{
	switch(vs->type){
		case CONST:
			SNPRINTF(buf, VSTACK_STR_SZ, "%s%d", ptr ? "" : "$", vs->bits.val);
			break;

		case FLAG:
			ICE("%s shouldn't be called with cmp-flag data", __func__);

		case LBL:
		{
			const int pic = fopt_mode & FOPT_PIC && vs->bits.lbl.pic;

			SNPRINTF(buf, VSTACK_STR_SZ, "%s%s",
					vs->bits.lbl.str,
					pic ? "(%rip)" : "");
			break;
		}

		case REG:
			snprintf(buf, VSTACK_STR_SZ, "%s%%%s%s",
					ptr ? "(" : "",
					reg_str_r(buf + 1 + ptr, vs),
					ptr ? ")" : "");
			break;

		case STACK:
		case STACK_SAVE:
		{
			int n = vs->bits.off_from_bp;
			SNPRINTF(buf, VSTACK_STR_SZ, "%s%d(%%rbp)", n < 0 ? "-" : "", abs(n));
			break;
		}
	}

	return buf;
}

static const char *vstack_str_r(char buf[VSTACK_STR_SZ], struct vstack *vs)
{
	return vstack_str_r_ptr(buf, vs, 0);
}

static const char *vstack_str(struct vstack *vs)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r(buf, vs);
}

static const char *vstack_str_ptr(struct vstack *vs, int ptr)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r_ptr(buf, vs, ptr);
}

static int alloc_stack(int sz)
{
	static int word_size;
	/* sz must be a multiple of word_size */

	if(!word_size)
		word_size = platform_word_size();

	if(sz){
		const int extra = sz % word_size ? word_size - sz % word_size : 0;
		out_asm("subq $%d, %%rsp", sz += extra);
	}

	return stack_sz + sz;
}

const char *call_reg_str(int i, type_ref *r)
{
	static char buf[REG_STR_SZ];
	const char suff[] = { call_regs[i].suffix, '\0' };
	const char *pre, *post;

	asm_reg_name(r, &pre, &post);

	if(!call_regs[i].suffix && r && type_ref_size(r, NULL) < type_primitive_size(type_long)){
		/* r9d, etc */
		snprintf(buf, sizeof buf, "r%cd", call_regs[i].reg);
	}else{
		snprintf(buf, sizeof buf,
				"%s%c%s", pre, call_regs[i].reg,
				*suff == *post ? post : suff);
	}

	return buf;
}

static void func_prologue(int stack_res, int nargs, int variadic)
{
	int arg_idx;

	out_asm("pushq %%rbp");
	out_asm("movq %%rsp, %%rbp");

	UCC_ASSERT(stack_sz == 0, "non-empty x86 stack for new func");

	if(nargs){
		int n_reg_args;

		n_reg_args = MIN(nargs, N_CALL_REGS);

		for(arg_idx = 0; arg_idx < n_reg_args; arg_idx++){
#define ARGS_PUSH

#ifdef ARGS_PUSH
			out_asm("push%c %%%s", asm_type_ch(NULL), call_reg_str(arg_idx, NULL));
#else
			stack_res += nargs * platform_word_size();

			out_asm("mov%c %%%s, -%d(%%rbp)",
					asm_type_ch(NULL),
					call_reg_str(arg_idx, NULL),
					platform_word_size() * (arg_idx + 1));
#endif
		}
	}

	stack_sz = impl.alloc_stack(stack_res)
		/* make room for saved args too */
		+ nargs * platform_word_size();

	if(variadic){
		/* play catchup, pushing any remaining reg args
		 * this is _after_ args and stack alloc,
		 * to simplify other offsetting code
		 */
		char *vfin = out_label_code("fin_...");

		for(; arg_idx < N_CALL_REGS; arg_idx++)
			out_asm("push%c %%%s", asm_type_ch(NULL), call_reg_str(arg_idx, NULL));

		out_asm("testb %%al, %%al");
		out_asm("jz %s", vfin);

		out_asm("// pushq %%xmm0 TODO");

		out_label(vfin);
		free(vfin);

		ICW("need to adjust stack_sz for variadics");
	}
}

static void func_epilogue(void)
{
	out_asm("leaveq");
	stack_sz = 0;
	out_asm("retq");
}

void impl_pop_func_ret(type_ref *r)
{
	(void)r;

	impl.load(vtop, REG_RET);
	vpop();
}

static const char *x86_cmp(struct flag_opts *flag)
{
	switch(flag->cmp){
#define OP(e, s, u) case flag_ ## e: return flag->is_signed ? s : u
		OP(eq, "e" , "e");
		OP(ne, "ne", "ne");
		OP(le, "le", "be");
		OP(lt, "l",  "b");
		OP(ge, "ge", "ae");
		OP(gt, "g",  "a");
#undef OP

		/*case flag_z:  return "z";
		case flag_nz: return "nz";*/
	}
	return NULL;
}

static void x86_load(struct vstack *from, const char *regstr)
{
	int lea = 0;

	switch(from->type){
		case FLAG:
			out_asm("set%s %%%s",
					x86_cmp(&from->bits.flag),
					regstr);
			return;

		case STACK:
		case LBL:
			lea = 1;
		case STACK_SAVE: /* voila */
		case CONST:
		case REG:
			break;
	}

	out_asm("%s%c %s, %%%s",
			lea ? "lea" : "mov",
			asm_type_ch(from->t),
			vstack_str(from),
			regstr);
}

static void load(struct vstack *from, int reg)
{
	char buf[REG_STR_SZ];
	type_ref *const save = from->t;

	if(from->type == REG && reg == from->bits.reg)
		return;

	x86_reg_str_r(buf, reg, from->t);

	if(from->type == FLAG){
		out_comment("zero for cmp");
		out_asm("mov%c $0, %%%s", asm_type_ch(from->t), buf);

		from->t = type_ref_new_CHAR(); /* force set%s to set the low byte */
		/* decl changed, reload the register name */
		x86_reg_str_r(buf, reg, from->t);
	}

	x86_load(from, buf);

	if(from->t != save)
		type_ref_free_1(from->t);

	v_clear(from, save);
	from->type = REG;
	from->bits.reg = reg;
}

static void store(struct vstack *from, struct vstack *to)
{
	char buf[VSTACK_STR_SZ];

	/* from must be either a reg, value or flag */
	if(from->type == FLAG && to->type == REG){
		/* setting a register from a flag - easy */
		impl.load(from, to->bits.reg);
		return;
	}

	if(from->type != CONST)
		v_to_reg(from);

	switch(to->type){
		case FLAG:
		case STACK_SAVE:
			ICE("invalid store %d", to->type);

		case REG:
		case CONST:
		case STACK:
		case LBL:
			out_asm("mov%c %s, %s",
					asm_type_ch(from->t),
					vstack_str_r(buf, from),
					vstack_str_ptr(to, 1));
			break;
	}
}

static void reg_swp(struct vstack *a, struct vstack *b)
{
	char bufa[REG_STR_SZ], bufb[REG_STR_SZ];
	int tmp;

	UCC_ASSERT(a->type == b->type && a->type == REG,
			"%s without regs (%d and %d)", __func__, a->type, b->type);

	out_asm("xchg %%%s, %%%s",
			reg_str_r(bufa, a),
			reg_str_r(bufb, b));

	tmp = a->bits.reg;
	a->bits.reg = b->bits.reg;
	b->bits.reg = tmp;
}

static void reg_cp(struct vstack *from, int r)
{
	char buf_v[VSTACK_STR_SZ];
	char buf_r[REG_STR_SZ];

	if(from->type == REG && from->bits.reg == r)
		return;

	x86_reg_str_r(buf_r, r, from->t);

	out_asm("mov%c %s, %%%s",
			asm_type_ch(from->t),
			vstack_str_r(buf_v, from),
			buf_r);
}

static void op(enum op_type op)
{
	const char *opc;

	switch(op){
#define OP(e, s) case op_ ## e: opc = s; break
		OP(multiply, "imul");
		OP(plus,     "add");
		OP(minus,    "sub");
		OP(xor,      "xor");
		OP(or,       "or");
		OP(and,      "and");
#undef OP

		case op_bnot:
		case op_not:
			ICE("unary op in binary");

		case op_shiftl:
		case op_shiftr:
		{
			char bufv[VSTACK_STR_SZ], bufs[VSTACK_STR_SZ];
			type_ref *free_this = NULL;

			/* value to shift must be a register */
			v_to_reg(&vtop[-1]);

			v_freeup_reg(REG_C, 2); /* shift by rcx... x86 sigh */

			switch(vtop->type){
				default:
					v_to_reg(vtop); /* TODO: v_to_reg_preferred(vtop, REG_C) */

				case REG:
					free_this = vtop->t = type_ref_new_CHAR();

					if(vtop->bits.reg != REG_C){
						impl.reg_cp(vtop, REG_C);
						vtop->bits.reg = REG_C;
					}
					break;

				case CONST:
					break;
			}

			vstack_str_r(bufs, vtop);
			vstack_str_r(bufv, &vtop[-1]);

			out_asm("%s%c %s, %s",
					op == op_shiftl ? "shl" : "shr",
					asm_type_ch(vtop[-1].t),
					bufs, bufv);

			vpop();

			type_ref_free_1(free_this);
			return;
		}

		case op_modulus:
		case op_divide:
		{
			/*
			 * divides the 64 bit integer EDX:EAX
			 * by the operand
			 * quotient  -> eax
			 * remainder -> edx
			 */
			int r_div;

			/*
			 * if we are using reg_[ad] elsewhere
			 * and they aren't queued for this idiv
			 * then save them, so we can use them
			 * for idiv
			 */

			/*
			 * Must freeup the lower
			 */
			v_freeup_regs(REG_A, REG_D);

			v_to_reg(vtop);
			r_div = v_to_reg(&vtop[-1]); /* TODO: similar to above - v_to_reg_preferred */

			if(r_div != REG_A){
				/* we already have rax in use by vtop, swap the values */
				if(vtop->type == REG && vtop->bits.reg == REG_A){
					impl.reg_swp(vtop, &vtop[-1]);
				}else{
					v_freeup_reg(REG_A, 2);
					impl.reg_cp(&vtop[-1], REG_A);
					vtop[-1].bits.reg = REG_A;
				}

				r_div = vtop[-1].bits.reg;
			}

			UCC_ASSERT(r_div == REG_A, "register A not chosen for idiv (%c)", regs[r_div]);

			out_asm("cqto");

			/* idiv takes either a reg or memory address */
			switch(vtop->type){
				default:
					v_to_reg(vtop);
					/* fall */

				case REG:
				case STACK:
					out_asm("idiv%c %s", asm_type_ch(vtop->t), vstack_str(vtop));
			}

			vpop();

			vtop_clear(vtop->t);
			vtop->type = REG;

			if(type_ref_size(vtop->t, NULL) != type_primitive_size(type_int)){
#if 0
Operand-Size         Dividend  Divisor  Quotient  Remainder
8                    AX        r/m8     AL        AH
16                   DX:AX     r/m16    AX        DX
32                   EDX:EAX   r/m32    EAX       EDX
64                   RDX:RAX   r/m64    RAX       RDX

but gcc and clang promote to ints anyway...
#endif
				ICW("idiv incorrect - need to load al:ah/dx:ax/edx:eax for %s",
						type_ref_to_str(vtop->t));
			}

			vtop->bits.reg = op == op_modulus ? REG_D : REG_A;
			return;
		}

		case op_eq:
		case op_ne:
		case op_le:
		case op_lt:
		case op_ge:
		case op_gt:
		{
			const int is_signed = type_ref_is_signed(vtop->t);
			char buf[VSTACK_STR_SZ];
			int inv = 0;

			vtop2_prepare_op();

			/* if we have a const, it must be the first arg */
			if(vtop[-1].type == CONST){
				vswap();
				inv = 1;
			}

			out_asm("cmp%c %s, %s",
					asm_type_ch(vtop[-1].t), /* pick the non-const one (for type-ing) */
					vstack_str(       vtop),
					vstack_str_r(buf, vtop - 1));

			vpop();
			vtop_clear(type_ref_new_BOOL()); /* cmp creates an int/bool */
			vtop->type = FLAG;
			vtop->bits.flag.cmp = op_to_flag(op);
			vtop->bits.flag.is_signed = is_signed;
			if(inv)
				v_inv_cmp(vtop);
			return;
		}

		case op_orsc:
		case op_andsc:
			ICE("%s shouldn't get here", op_to_str(op));

		default:
			ICE("invalid op %s", op_to_str(op));
	}

	{
		char buf[VSTACK_STR_SZ];

		vtop2_prepare_op();

		/* TODO: -O1
		 * if the op is commutative and we have REG_RET,
		 * make it the result reg
		 */

		out_asm("%s%c %s, %s", opc,
				asm_type_ch(vtop->t),
				vstack_str_r(buf, &vtop[ 0]),
				vstack_str(       &vtop[-1]));

		/* remove first operand - result is then in vtop (already in a reg) */
		vpop();
	}
}

static void deref()
{
	const int r = v_unused_reg(1); /* allocate a reg here first, since it'll be used later too */
	char ptr[VSTACK_STR_SZ], dst[REG_STR_SZ];

	/* optimisation: if we're dereffing a pointer to stack/lbl, just do a mov */
	switch(vtop->type){
		case LBL:
		case STACK:
		case CONST:
			v_deref_decl(vtop);
			out_asm("mov%c %s, %%%s",
					asm_type_ch(vtop->t),
					vstack_str_r_ptr(ptr, vtop, 1),
					x86_reg_str_r(dst, r, vtop->t));
			break;

		default:
			v_to_reg(vtop);

			vstack_str_r_ptr(ptr, vtop, 1);

			/* loaded the pointer, now we apply the deref change */
			v_deref_decl(vtop);
			x86_reg_str_r(dst, r, vtop->t);

			out_asm("mov%c %s, %%%s",
					asm_type_ch(vtop->t),
					ptr, dst);
			break;
	}

	vtop->type = REG;
	vtop->bits.reg = r;
}

static void op_unary(enum op_type op)
{
	const char *opc;

	v_prepare_op(vtop);

	switch(op){
		default:
			ICE("invalid unary op %s", op_to_str(op));

		case op_plus:
			/* noop */
			return;

#define OP(o, s) case op_ ## o: opc = #s; break
		OP(minus, neg);
		OP(bnot, not);
#undef OP

		case op_not:
			out_push_i(vtop->t, 0);
			out_op(op_eq);
			return;
	}

	out_asm("%s %s", opc, vstack_str(vtop));
}

static void normalise(void)
{
	char buf[REG_STR_SZ];

	if(vtop->type != REG)
		v_to_reg(vtop);

	out_comment("normalise");

	out_asm("and%c $0x1, %%%s",
			asm_type_ch(vtop->t),
			reg_str_r(buf, vtop));
}

void impl_cast(type_ref *from, type_ref *to)
{
	int szfrom, szto;

	szfrom = asm_type_size(from);
	szto   = asm_type_size(to);

	if(szfrom != szto){
		if(szfrom < szto){
			const int is_signed = type_ref_is_signed(from);
			const int int_sz = type_primitive_size(type_int);

			char buf_from[REG_STR_SZ], buf_to[REG_STR_SZ];

			v_to_reg(vtop);

			x86_reg_str_r(buf_from, vtop->bits.reg, from);

			if(!is_signed
			&& type_ref_size(to,   NULL) > int_sz
			&& type_ref_size(from, NULL) == int_sz)
			{
				/*
				 * movzx %eax, %rax is invalid
				 * since movl %eax, %eax automatically zeros the top half of rax
				 * in x64 mode
				 */
				out_asm("movl %%%s, %%%s", buf_from, buf_from);
				return;
			}else{
				x86_reg_str_r(buf_to, vtop->bits.reg, to);
			}

			out_asm("mov%cx %%%s, %%%s", "zs"[is_signed], buf_from, buf_to);
		}else{
			char buf[TYPE_REF_STATIC_BUFSIZ];

			out_comment("truncate cast from %s to %s, size %d -> %d",
					from ? type_ref_to_str_r(buf, from) : "",
					to   ? type_ref_to_str(to) : "",
					szfrom, szto);
		}
	}
}

static const char *x86_call_jmp_target(struct vstack *vp, int no_rax)
{
	static char buf[VSTACK_STR_SZ + 2];

	switch(vp->type){
		case LBL:
			return vp->bits.lbl.str;

		case STACK:
			/* jmp *-8(%rbp) */
			*buf = '*';
			vstack_str_r(buf + 1, vp);
			return buf;

		case STACK_SAVE:
		case FLAG:
		case REG:
		case CONST:
			v_to_reg(vp); /* again, v_to_reg_preferred(), except that we don't want a reg */

			if(no_rax && vp->bits.reg == REG_A){
				int r = v_unused_reg(1);
				impl.reg_cp(vp, r);
				vp->bits.reg = r;
			}

			strcpy(buf, "*%");
			reg_str_r(buf + 2, vp);

			return buf;
	}

	ICE("invalid jmp target");
	return NULL;
}

static void jmp()
{
	out_asm("jmp %s", x86_call_jmp_target(vtop, 0));
}

static void jcond(int true, const char *lbl)
{
	switch(vtop->type){
		case FLAG:
			UCC_ASSERT(true, "jcond(false) for flag - should've been inverted");

			out_asm("j%s %s", x86_cmp(&vtop->bits.flag), lbl);
			break;

		case CONST:
			if(true == !!vtop->bits.val){
				out_asm("jmp %s", lbl);
				out_comment("// constant jmp condition %d", vtop->bits.val);
			}
			break;

		case STACK:
		case STACK_SAVE:
		case LBL:
			v_to_reg(vtop);

		case REG:
		{
			char buf[REG_STR_SZ];

			reg_str_r(buf, vtop);

			out_asm("test %%%s, %%%s", buf, buf);
			out_asm("j%sz %s", true ? "n" : "", lbl);
		}
	}
}

void impl_call(const int nargs, type_ref *r_ret, type_ref *r_func)
{
#define INC_NFLOATS(t) if(t && type_ref_is_floating(t)) ++nfloats

	int i, ncleanup;
	int nfloats = 0;

	/* pre-scan of arguments - eliminate flags
	 * (should only be one,
	 * since we can only have one flag at a time)
	 */
	for(i = 0; i < MIN(nargs, N_CALL_REGS); i++)
		if(vtop->type == FLAG){
			v_to_reg(vtop);
			break;
		}

	for(i = 0; i < MIN(nargs, N_CALL_REGS); i++){
		int ri;

		ri = call_regs[i].idx;
		if(ri != -1)
			v_freeup_reg(ri, 1);

		INC_NFLOATS(vtop->t);

		x86_load(vtop, call_reg_str(i, vtop->t));
		vpop();
	}
	/* push remaining args onto the stack */
	ncleanup = nargs - i;
	for(; i < nargs; i++){
		INC_NFLOATS(vtop->t);

		/* can't push non-word sized vtops */
		if(vtop->t && type_ref_size(vtop->t, NULL) != platform_word_size())
			out_cast(vtop->t, type_ref_new_VOID_PTR());

		out_asm("pushq %s", vstack_str(vtop));
		vpop();
	}

	/* save all registers */
#define vcond (&vstack[i] < vtop)
	i = 0;
	if(vcond)
		out_comment("pre-call reg-save");
	for(; vcond; i++)
		/* TODO: v_to_mem (__asm__ branch) */
		if(vstack[i].type == REG || vstack[i].type == FLAG)
			v_save_reg(&vstack[i]);

	{
		funcargs *args = type_ref_funcargs(r_func);
		int need_float_count = args->variadic || (!args->arglist && !args->args_void);
		const char *jtarget = x86_call_jmp_target(vtop, need_float_count);

		/* if x(...) or x() */
		if(need_float_count)
			out_asm("movb $%d, %%al", nfloats); /* we can never have a funcptr in rax, so we're fine */

		out_asm("callq %s", jtarget);
	}

	if(ncleanup)
		out_asm("addq $%d, %%rsp", ncleanup * platform_word_size());

	/* return type */
	vtop_clear(r_ret);
	vtop->type = REG;
	vtop->bits.reg = REG_RET;
}

static void undefined(void)
{
	out_asm("ud2");
}

static int frame_ptr_to_reg(int nframes)
{
	char buf[REG_STR_SZ];
	int r = v_unused_reg(1);

	x86_reg_str_r(buf, r, NULL);

	out_asm("movq %%rbp, %%%s", buf);
	while(--nframes > 0)
		out_asm("movq (%%%s), %%%s", buf, buf);

	return r;
}

void impl_x86_64()
{
	N_REGS = 4;
	N_CALL_REGS = 6;
	REG_RET = REG_A;

	reserved_regs = umalloc(N_REGS * sizeof *reserved_regs);

#define INIT(fn) impl.fn = fn
	INIT(store);
	INIT(load);
	INIT(reg_cp);
	INIT(reg_swp);
	INIT(op);
	INIT(op_unary);
	INIT(deref);
	INIT(normalise);
	INIT(jmp);
	INIT(jcond);
	/*INIT(cast);*/
	/*INIT(call);*/
	INIT(alloc_stack);
	INIT(func_prologue);
	INIT(func_epilogue);
	INIT(undefined);
	INIT(frame_ptr_to_reg);
}
