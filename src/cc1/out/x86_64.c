#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "../../util/util.h"
#include "../../util/alloc.h"
#include "../../util/platform.h"
#include "../data_structs.h"
#include "vstack.h"
#include "x86_64.h"
#include "../cc1.h"
#include "asm.h"
#include "common.h"
#include "out.h"

#ifndef MIN
#  define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

#define REG_STR_SZ 8

#define REG_RET REG_A

#define VSTACK_STR_SZ 128

enum p_opts
{
	P_NO_INDENT = 1 << 0,
	P_NO_NL     = 1 << 1
};

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


static void out_asm2(enum p_opts opts, const char *fmt, ...) ucc_printflike(2, 3);

static void out_asmv(enum p_opts opts, const char *fmt, va_list l)
{
	FILE *f = cc_out[SECTION_TEXT];

	if((opts & P_NO_INDENT) == 0)
		fputc('\t', f);

	vfprintf(f, fmt, l);

	if((opts & P_NO_NL) == 0)
		fputc('\n', f);
}

void out_asm(const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	out_asmv(0, fmt, l);
	va_end(l);
}

static void out_asm2(enum p_opts opts, const char *fmt, ...)
{
	va_list l;
	va_start(l, fmt);
	out_asmv(opts, fmt, l);
	va_end(l);
}

void impl_comment(const char *fmt, va_list l)
{
	out_asm2(P_NO_NL, "// ");
	out_asmv(P_NO_INDENT, fmt, l);
}

void impl_lbl(const char *lbl)
{
	out_asm2(P_NO_INDENT, "%s:", lbl);
}

static const char *x86_reg_str_r(char buf[REG_STR_SZ], int reg, decl *d)
{
	const char *regpre, *regpost;

	UCC_ASSERT((unsigned)reg < N_REGS, "invalid x86 reg %d", reg);

	asm_reg_name(d, &regpre, &regpost);

	snprintf(buf, REG_STR_SZ, "%s%c%s", regpre, regs[reg], regpost);

	return buf;
}

static const char *reg_str_r(char buf[REG_STR_SZ], struct vstack *reg)
{
	UCC_ASSERT(reg->type == REG, "non-reg %d", reg->type);
	return x86_reg_str_r(buf, reg->bits.reg, reg->d);
}

static const char *vstack_str_r(char buf[VSTACK_STR_SZ], struct vstack *vs)
{
	switch(vs->type){
		case CONST:
			SNPRINTF(buf, VSTACK_STR_SZ, "%s%d",
					vs->is_addrof ? "" : "$", vs->bits.val);
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
			*buf = '%';
			reg_str_r(buf + 1, vs);
			break;

		case STACK:
		{
			int n = vs->bits.off_from_bp;
			SNPRINTF(buf, VSTACK_STR_SZ, "%s0x%x(%%rbp)", n < 0 ? "-" : "", abs(n));
			break;
		}
	}

	return buf;
}

const char *vstack_str(struct vstack *vs)
{
	static char buf[VSTACK_STR_SZ];
	return vstack_str_r(buf, vs);
}

int impl_alloc_stack(int sz)
{
	static int word_size;
	/* sz must be a multiple of word_size */

	if(!word_size)
		word_size = platform_word_size();

	if(sz)
		out_asm("subq $0x%x, %%rsp", sz + word_size - sz % word_size);

	return sz + stack_sz;
}

void impl_free_stack(int sz)
{
	if(sz)
		out_asm("addq $0x%x, %%rsp", sz);
}

const char *call_reg_str(int i, decl *d)
{
	static char buf[REG_STR_SZ];
	const char suff[] = { call_regs[i].suffix, '\0' };
	const char *pre, *post;

	asm_reg_name(d, &pre, &post);

	if(!call_regs[i].suffix && d && decl_size(d) < type_primitive_size(type_long)){
		/* r9d, etc */
		snprintf(buf, sizeof buf, "r%cd", call_regs[i].reg);
	}else{
		snprintf(buf, sizeof buf,
				"%s%c%s", pre, call_regs[i].reg,
				*suff == *post ? post : suff);
	}

	return buf;
}

void impl_func_prologue(int stack_res, int nargs, int variadic)
{
	(void)nargs;

	out_asm("pushq %%rbp");
	out_asm("movq %%rsp, %%rbp");

	if(stack_res){
		int i, n_reg_args;

		UCC_ASSERT(stack_sz == 0, "non-empty x86 stack for new func");

		stack_sz = impl_alloc_stack(stack_res);

		if(!variadic){
			n_reg_args = MIN(nargs, N_CALL_REGS);

			for(i = 0; i < n_reg_args; i++){
				out_asm("mov%c %%%s, -0x%x(%%rbp)",
						asm_type_ch(NULL),
						call_reg_str(i, NULL),
						platform_word_size() * (i + 1));
			}
		}
	}
}

void impl_func_epilogue(void)
{
	out_asm("leaveq");
	stack_sz = 0;
	out_asm("retq");
}

void impl_pop_func_ret(decl *d)
{
	(void)d;

	impl_load(vtop, REG_RET);
	vpop();
}

static const char *x86_cmp(enum flag_cmp cmp, decl *d)
{
	const int is_signed = d->type->is_signed;

	switch(cmp){
#define OP(e, s, u) case flag_ ## e: return is_signed ? s : u
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

static enum flag_cmp op_to_flag(enum op_type op)
{
	switch(op){
#define OP(x) case op_ ## x: return flag_ ## x
		OP(eq);
		OP(ne);
		OP(le);
		OP(lt);
		OP(ge);
		OP(gt);
#undef OP

		default:
			break;
	}

	ICE("invalid op");
	return -1;
}

static void x86_load(struct vstack *from, const char *regstr)
{
	switch(from->type){
		case FLAG:
			out_asm("set%s %%%s",
					x86_cmp(from->bits.flag, from->d),
					regstr);
			return;

		default:
			break;
	}

	if(from->is_addrof && from->type == REG){
		out_asm("mov%c (%s), %%%s",
				asm_type_ch(from->d),
				vstack_str(from),
				regstr);
	}else{
		out_asm("%s%c %s, %%%s",
				from->is_addrof ? "lea" : "mov",
				asm_type_ch(from->d),
				vstack_str(from),
				regstr);
	}
}

void impl_load(struct vstack *from, int reg)
{
	char buf[REG_STR_SZ];
	decl *const save = from->d;

	if(!from->is_addrof && from->type == REG && reg == from->bits.reg)
		return;

	x86_reg_str_r(buf, reg, from->d);

	if(from->type == FLAG){
		out_asm("mov%c $0, %%%s // zero for cmp", asm_type_ch(from->d), buf);

		from->d = decl_new_char(); /* force set%s to set the low byte */
		/* decl changed, reload the register name */
		x86_reg_str_r(buf, reg, from->d);
	}

	x86_load(from, buf);

	if(from->d != save)
		decl_free(from->d);

	v_clear(from, save);
	from->type = REG;
	from->bits.reg = reg;
}

void impl_store(struct vstack *from, struct vstack *to)
{
	char buf[VSTACK_STR_SZ];

	/* from must be either a reg, value or flag */
	if(from->type == FLAG && to->type == REG){
		/* setting a register from a flag - easy */
		impl_load(from, to->bits.reg);
		return;
	}

	/* try not to pull into a reg if at all possible */
	if(to->is_addrof && from->type != REG && from->is_addrof)
		v_to_reg(from);

	/* if to a register, we can assign straight to it */
	switch(to->type){
		case FLAG:
			ICE("invalid store FLAG");

		case REG:
reg_store:
			out_asm("mov%c %s, %s%s%s",
					asm_type_ch(from->d),
					vstack_str_r(buf, from),
					to->is_addrof ? "(" : "",
					vstack_str(to),
					to->is_addrof ? ")" : ""
					);
			break;

		case CONST:
			UCC_ASSERT(to->is_addrof, "can't store to a value");

			out_asm("mov%c %s, %s",
					asm_type_ch(from->d),
					vstack_str_r(buf, from),
					vstack_str(to));
			break;

		case STACK:
		case LBL:
			if(to->is_addrof){
				/* storing to that stack location - apply the change here for store */
				to->is_addrof = 0;
			}else{
				/* storing to a value pointed to by the stack/a label, load it in */
				v_to_reg(to);
				to->is_addrof = 1;
			}
			goto reg_store;
	}
}

void impl_reg_swp(struct vstack *a, struct vstack *b)
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

void impl_reg_cp(struct vstack *from, int r)
{
	char buf_v[VSTACK_STR_SZ];
	char buf_r[REG_STR_SZ];

	UCC_ASSERT(from->type == REG, "reg_cp: not a reg");

	if(from->bits.reg == r)
		return;

	x86_reg_str_r(buf_r, r, from->d);

	out_asm("mov%c %s, %%%s",
			asm_type_ch(from->d),
			vstack_str_r(buf_v, from),
			buf_r);
}

void impl_op(enum op_type op)
{
	const char *opc;
	int normalise = 0;

	switch(op){
#define OP(e, s) case op_ ## e: opc = s; break
		OP(multiply, "imul");
		OP(plus,     "add");
		OP(minus,    "sub");
		OP(xor,      "xor");
		OP(or,       "or");
		OP(and,      "and");

		case op_not:
		normalise = 1;

		OP(bnot,     "not");
#undef OP

		case op_shiftl:
		case op_shiftr:
		{
			char bufv[VSTACK_STR_SZ], bufs[VSTACK_STR_SZ];
			decl *free_this = NULL;

			/* value to shift must be a register */
			v_to_reg(&vtop[-1]);

			v_freeup_reg(REG_C, 2); /* shift by rcx... x86 sigh */

			switch(vtop->type){
				default:
					v_to_reg(vtop); /* TODO: v_to_reg_preferred(vtop, REG_C) */

				case REG:
					free_this = vtop->d = decl_new_char();

					if(vtop->bits.reg != REG_C){
						impl_reg_cp(vtop, REG_C);
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
					asm_type_ch(vtop[-1].d),
					bufs, bufv);

			vpop();

			if(free_this)
				decl_free(free_this);
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

			vtop2_prepare_op();

			/*
			 * if we are using reg_[ad] elsewhere
			 * and they aren't queued for this idiv
			 * then save them, so we can use them
			 * for idiv
			 */
			v_freeup_reg(REG_A, 2);
			v_freeup_reg(REG_D, 2);

			v_to_reg(vtop);
			r_div = v_to_reg(&vtop[-1]); /* TODO: similar to above - v_to_reg_preferred */

			if(r_div != REG_A){
				/* we already have rax in use by vtop, swap the values */
				impl_reg_swp(vtop, &vtop[-1]);

				r_div = vtop[-1].bits.reg;
			}

			UCC_ASSERT(r_div == REG_A, "register A not chosen for idiv");

			out_asm("cqto");

			/* idiv takes either a reg or memory address */
			switch(vtop->type){
				default:
					v_to_reg(vtop);
					/* fall */

				case REG:
				case STACK:
					out_asm("idiv%c %s", asm_type_ch(vtop->d), vstack_str(vtop));
			}

			vpop();

			vtop_clear(vtop->d);
			vtop->type = REG;
			/* FIXME */
			if(vtop->d->type->primitive != type_int){
				ICE("idiv incorrect - need to load ax:al/ax:dx/eax:edx for %s",
						decl_to_str(vtop->d));
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
			char buf[VSTACK_STR_SZ];

			vtop2_prepare_op();

			/*
			 * if we have a const, it must be the first arg
			 * not sure why this works without having to
			 * invert the comparison
			 */
			if(vtop->type == CONST)
				vswap();

			out_asm("cmp%c %s, %s",
					asm_type_ch(vtop->d),
					vstack_str(&vtop[-1]),
					vstack_str_r(buf, vtop));

			vpop();
			vtop_clear(decl_new_type(type_int)); /* cmp creates an int */
			vtop->type = FLAG;
			vtop->bits.flag = op_to_flag(op);
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

		out_asm("%s%c %s, %s", opc,
				asm_type_ch(vtop->d),
				vstack_str_r(buf, &vtop[ 0]),
				vstack_str(       &vtop[-1]));

		/* remove first operand - result is then in vtop (already in a reg) */
		vpop();

		if(normalise)
			out_normalise();
	}
}

void impl_deref()
{
	char ptr[REG_STR_SZ], dst[REG_STR_SZ];
	int r;

	v_to_reg(vtop);
	r = v_unused_reg(1);

	x86_reg_str_r(ptr, vtop->bits.reg, NULL);
	/* loaded the pointer, now we apply the deref change */
	v_deref_decl(vtop);
	x86_reg_str_r(dst, r, vtop->d);

	out_asm("mov%c (%%%s), %%%s",
			asm_type_ch(vtop->d),
			ptr, dst);

	vtop->bits.reg = r;
}

void impl_op_unary(enum op_type op)
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
		OP(not, not);
		OP(minus, neg);
		OP(bnot, not);
#undef OP
	}

	out_asm("%s %s", opc, vstack_str(vtop));

	if(op == op_not)
		out_normalise();
}

void impl_normalise(void)
{
	char buf[REG_STR_SZ];

	if(vtop->type != REG)
		v_to_reg(vtop);

	out_comment("normalise");

	out_asm("and%c $0x1, %%%s",
			asm_type_ch(vtop->d),
			reg_str_r(buf, vtop));
}

void impl_cast(decl *from, decl *to)
{
	int szfrom, szto;

	szfrom = asm_type_size(from);
	szto   = asm_type_size(to);

	if(szfrom != szto){
		if(szfrom < szto){
			const int is_signed = from->type->is_signed;
			const int int_sz = type_primitive_size(type_int);

			char buf_from[REG_STR_SZ], buf_to[REG_STR_SZ];

			v_to_reg(vtop);

			x86_reg_str_r(buf_from, vtop->bits.reg, from);

			if(!is_signed && decl_size(to) > int_sz && decl_size(from) == int_sz){
				/*
				 * movzx %eax, %rax is invalid
				 * since movl %eax, %eax automatically zeros the top half of rax
				 * in x64 mode
				 */
				out_asm("movl %%%s, %%%s", buf_from, buf_from);
				return;
			}else{
				x86_reg_str_r(buf_to,   vtop->bits.reg, to);
			}

			out_asm("mov%cx %%%s, %%%s", "zs"[is_signed], buf_from, buf_to);
		}else{
			char buf[DECL_STATIC_BUFSIZ];

			out_comment("truncate cast from %s to %s, size %d -> %d",
					decl_to_str_r(buf, from), decl_to_str(to),
					szfrom, szto);
		}
	}
}

static const char *x86_call_jmp_target(struct vstack *vp)
{
	static char buf[VSTACK_STR_SZ + 2];

	switch(vp->type){
		case LBL:
			if(vp->is_addrof)
				return vp->bits.lbl.str;
			/* fall */

		case STACK:
			/* jmp *-8(%rbp) */
			if(!vp->is_addrof){
				*buf = '*';
				vstack_str_r(buf + 1, vp);
				return buf;
			}
			/* fall */

		case FLAG:
		case REG:
		case CONST:
			strcpy(buf, "*%");
			v_to_reg(vp);
			reg_str_r(buf + 2, vp);

			return buf;
	}

	ICE("invalid jmp target");
	return NULL;
}

void impl_jmp()
{
	out_asm("jmp %s", x86_call_jmp_target(vtop));
}

void impl_jcond(int true, const char *lbl)
{
	switch(vtop->type){
		case FLAG:
			UCC_ASSERT(true, "jcond(false) for flag - should've been inverted");

			out_asm("j%s %s",
					x86_cmp(vtop->bits.flag, vtop->d),
					lbl);
			break;

		case CONST:
			if(true == !!vtop->bits.val)
				out_asm("jmp %s // constant jmp condition %d", lbl, vtop->bits.val);
			break;

		case STACK:
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

void impl_call(const int nargs, int variadic, decl *d)
{
	int i, ncleanup;

	if(variadic){
		for(i = nargs - 1; i >=0; i--)
			out_asm("pushq %s", vstack_str(&vtop[-i]));
		for(i = 0; i < nargs; i++)
			vpop();
		ncleanup = nargs;
	}else{
		for(i = 0; i < MIN(nargs, N_CALL_REGS); i++){
			int ri;

			ri = call_regs[i].idx;
			if(ri != -1)
				v_freeup_reg(ri, 1);

			x86_load(vtop, call_reg_str(i, vtop->d));
			vpop();
		}
		/* push remaining args onto the stack */
		ncleanup = nargs - i;
		for(; i < nargs; i++){
			out_asm("pushq %s", vstack_str(vtop));
			vpop();
		}
	}

	/* save all registers */
#define vcond (&vstack[i] < vtop)
	i = 0;
	if(vcond)
		out_comment("pre-call reg-save");
	for(; vcond; i++)
		if(vstack[i].type == REG)
			v_save_reg(&vstack[i]);

	out_asm("callq %s", x86_call_jmp_target(vtop));

	if(ncleanup)
		impl_free_stack(ncleanup * platform_word_size());

	/* return type */
	vtop_clear(d);
	vtop->type = REG;
	vtop->bits.reg = REG_RET;
}

void impl_undefined(void)
{
	out_asm("ud2");
}

int impl_frame_ptr_to_reg(int nframes)
{
	char buf[REG_STR_SZ];
	int r = v_unused_reg(1);

	x86_reg_str_r(buf, r, NULL);

	out_asm("movq %%rbp, %%%s", buf);
	while(--nframes > 0)
		out_asm("movq (%%%s), %%%s", buf, buf);

	return r;
}
