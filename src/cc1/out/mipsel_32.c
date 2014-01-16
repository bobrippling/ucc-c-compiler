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

#define SYMBOLIC_REGS

#define F_DEBUG cc_out[SECTION_TEXT]
#define REG_STR_SZ 4 /* r[0-31] / r[a-z][0-9a-z] */

#define LBL_CHECK(vp)                  \
	do{                                  \
			static int w;                    \
			if(!w){                          \
				if(vp->bits.lbl.pic){          \
					ICW("TODO: pic on mips");    \
					w = 1;                       \
				}else if(vp->bits.lbl.offset){ \
					ICW("TODO: offset on mips"); \
					w = 1;                       \
				}                              \
			}                                \
	}while(0)

#define N_CALL_REGS 4 /* FIXME: todo: stack args */

#pragma GCC diagnostic ignored "-Wunused-parameter"

static const char *const sym_regs[] = {
	"0", "at",
	"v0", "v1",
	"a0", "a1", "a2", "a3",
	"t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
	"s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
	"t8", "t9",
	"k0", "k1",
	"gp",
	"sp", "fp", "ra"
};

const struct asm_type_table asm_type_table[ASM_TABLE_LEN] = {
	{ 1,  'b',  "byte"  },
	{ 2,  'w',  "word"  },
	{ 4,  '\0', "long"  },
	{ -1,   0,   NULL   },
};

static const int call_regs[] = {
	MIPS_REG_A0,
	MIPS_REG_A1,
	MIPS_REG_A2,
	MIPS_REG_A3,
};

static char *reg_str_r_i(char buf[REG_STR_SZ], int r)
{
#ifdef SYMBOLIC_REGS
	strcpy(buf, sym_regs[r]);
#else
	snprintf(buf, REG_STR_SZ, "r%d", r);
#endif
	return buf;
}

static char *reg_str_r(char buf[REG_STR_SZ], struct vstack *vp)
{
	UCC_ASSERT(vp->type == REG, "not reg");
	return reg_str_r_i(buf, vp->bits.reg);
}

static char *reg_str_i(int r)
{
	static char buf[REG_STR_SZ];
	return reg_str_r_i(buf, r);
}

int impl_reg_to_scratch(int i)
{
	if(MIPS_REG_TMP_0 <= i && i <= MIPS_REG_TMP_7)
		return i - MIPS_REG_TMP_0;

	if(MIPS_REG_TMP_8 <= i && i <= MIPS_REG_TMP_9)
		return i - MIPS_REG_TMP_8 + (MIPS_REG_TMP_7 - MIPS_REG_TMP_0);

	ICE("bad scratch register");
}

int impl_scratch_to_reg(int i)
{
	if(i > 8)
		return MIPS_REG_TMP_8 + i - 8;
	return MIPS_REG_TMP_0 + i;
}

int impl_n_call_regs(type *fr)
{
	(void)fr;
	return N_CALL_REGS;
}

void impl_func_prologue_save_fp(void)
{
	out_asm("addi  $sp, $sp, -8"); /* space for saved ret and fp */
	out_asm("sw    $ra, 4($sp)");  /* save ret */
	out_asm("sw    $fp,  ($sp)");  /* save fp */
	out_asm("move  $fp, $sp");     /* new frame */
}

void impl_func_prologue_save_call_regs(type *rf, int nargs)
{
	const unsigned pws = platform_word_size();
	int i;

	UCC_ASSERT(nargs <= N_CALL_REGS, "TODO: more than 4 args"); /* FIXME */

	for(i = 0; i < nargs; i++){
		struct vstack arg   = VSTACK_INIT(REG),
		              stack = VSTACK_INIT(STACK);

		arg.bits.reg = call_regs[i];
		stack.bits.off_from_bp = -(i + 1) * pws;
		/* +1 for the saved fp */

		impl_store(&arg, &stack);
	}
}

int impl_func_prologue_save_variadic(type *rf, int nargs)
{
	(void)rf;
	ICW("variadic function prologue on MIPS");
	return 0; /* FIXME */
}

void impl_func_epilogue(type *fr)
{
	out_asm("move $sp, $fp");
	out_asm("lw $fp,  ($sp)");
	out_asm("lw $ra, 4($sp)");
	out_asm("addi $sp, $sp, 8");
	out_asm("j $ra");
}

void impl_lea(struct vstack *from, int reg)
{
	char rstr[REG_STR_SZ];

	reg_str_r_i(rstr, reg);

	switch(from->type){
		case FLAG:
			ICE("FLAG in MIPS");
		case CONST:
		case REG:
		case STACK_SAVE:
			ICE("%s of %d", __func__, from->type);

		case STACK:
			out_asm("la $%s, %d($fp)", rstr, from->bits.off_from_bp);
			break;
		case LBL:
			LBL_CHECK(from);
			out_asm("la $%s, %s", rstr, from->bits.lbl.str);
			break;
	}
}

static const char *mips_type_ch(type *ty, int chk_sig)
{
	int sz, sig;

	if(!ty)
		return "w";

	sz = type_size(ty, NULL);
	sig = chk_sig ? type_is_signed(ty) : 1;

	switch(sz){
		case 1:
			return sig ? "b" : "bu";
		case 2:
			return sig ? "h" : "hu";
		case 4:
			return "w"; /* full 32-bits,
										 no need to sign extend a load */
	}

	ICE("bad type ref size for load %d (%d)", sz, sig);
}

void impl_load(struct vstack *from, int reg)
{
	char rstr[REG_STR_SZ];

	reg_str_r_i(rstr, reg);

	switch(from->type){
		case FLAG:
			ICE("FLAG in MIPS");

		case LBL:
			LBL_CHECK(from);
			out_asm("l%s $%s, %s", mips_type_ch(from->t, 1),
					rstr, from->bits.lbl.str);
			break;

		case CONST:
			if(from->bits.val == 0)
				out_asm("move $%s, $%s", rstr, sym_regs[MIPS_REG_ZERO]);
			else
				out_asm("li%s $%s, %ld",
						type_is_signed(from->t) ? "" : "u",
						rstr, from->bits.val);
			break;

		case REG:
			impl_reg_cp(from, reg);
			break;

		case STACK:
		case STACK_SAVE:
			out_asm("l%s $%s, %d($fp)",
					mips_type_ch(from->t, 1),
					rstr,
					from->bits.off_from_bp);
	}
}

void impl_store(struct vstack *from, struct vstack *to)
{
	char rstr[REG_STR_SZ];
	const char *ty_ch = mips_type_ch(from->t, 0);

	v_to_reg(from); /* room for optimisation, e.g. const */
	reg_str_r(rstr, from);

	switch(to->type){
		case FLAG:
			ICE("FLAG in MIPS");

		case STACK_SAVE:
			ICE("store to %d", to->type);

		case LBL:
			LBL_CHECK(from);
			out_asm("s%s $%s, %s",
					ty_ch,
					rstr, to->bits.lbl.str);
			break;

		case CONST:
			out_asm("s%s $%s, %ld",
					ty_ch, rstr,
					to->bits.val);
			break;

		case REG:
			out_asm("s%s $%s, ($%s)",
					ty_ch, rstr,
					reg_str_i(to->bits.reg));
			break;

		case STACK:
			out_asm("s%s $%s, %d($fp)",
					ty_ch, rstr,
					to->bits.off_from_bp);
	}
}

static void impl_reg_cp_reg(int from, int to)
{
	char rstr[REG_STR_SZ];

	out_asm("move $%s, $%s",
			reg_str_i(to),
			reg_str_r_i(rstr, from));
}

void impl_reg_swp(struct vstack *va, struct vstack *vb)
{
	const int r = v_unused_reg(1),
	          a = va->bits.reg,
	          b = vb->bits.reg;

	impl_reg_cp_reg(a, r);
	impl_reg_cp_reg(b, a);
	impl_reg_cp_reg(r, b);
}

void impl_reg_cp(struct vstack *from, int r)
{
	if(from->type == REG && from->bits.reg == r) /* FIXME: x86_64 merge */
		return;

	impl_reg_cp_reg(from->bits.reg, r);
}

void impl_op(enum op_type op)
{
	const char *cmp, *immediate;
	char r_vtop[16], r_vtop1[REG_STR_SZ];
	const char *ssigned;

	v_to_reg(vtop - 1);
	reg_str_r(r_vtop1, vtop-1);

	v_to_reg_const(vtop);
	if(vtop->type == CONST){
		immediate = "i";
		snprintf(r_vtop, sizeof r_vtop, "%ld", vtop->bits.val);
	}else{
		immediate = "";
		reg_str_r(r_vtop, vtop);
	}

	ssigned = type_is_signed(vtop->t) ? "" : "u";

	switch(op){
#define OP(ty) case op_ ## ty: cmp = #ty; goto cmp
		OP(eq);
		OP(ne);
		OP(le);
		OP(lt);
		OP(ge);
		OP(gt);
#undef OP
cmp:
		out_asm("s%s%s%s $%s, $%s, $%s",
				cmp, immediate, ssigned, r_vtop1, r_vtop1, r_vtop);
		break;

		case op_orsc:
		case op_andsc:
		case op_not:
		case op_bnot:
		case op_unknown:
			ICE("%s", op_to_str(op));

#define OP(ty, n) case op_ ## ty: cmp = n; goto op
		OP(multiply, "mul");
		OP(plus,     "add");
		OP(minus,    "sub");
		OP(divide,   "div");
		OP(modulus,  "div"); /* special */
		OP(xor,      "xor");
		OP(or,       "or");
		OP(and,      "and");
		OP(shiftl,   "sll");
		OP(shiftr,   "srl");
#undef OP
op:
		out_asm("%s%s $%s, $%s, $%s",
				cmp, immediate,
				r_vtop1, r_vtop1, r_vtop);

		switch(op){
			case op_divide:
				out_asm("mflo $%s", r_vtop1);
				break;
			case op_modulus:
				out_asm("mfhi $%s", r_vtop1);
			default:
				break;
		}
		break;
	}

	vpop();
}

void impl_deref_reg()
{
	char rstr[REG_STR_SZ];

	UCC_ASSERT(vtop->type == REG, "not reg (%d)", vtop->type);

	reg_str_r_i(rstr, vtop->bits.reg);

	/* XXX: must be before mips_type_ch(), but after reg_str() */
	v_deref_decl(vtop);

	out_asm("l%s $%s, ($%s)", mips_type_ch(vtop->t, 1), rstr, rstr);
}

void impl_op_unary(enum op_type op)
{
	char r_vtop[REG_STR_SZ];

	v_to_reg(vtop);
	reg_str_r(r_vtop, vtop);

	switch(op){
		default:
			ICE("unary %s", op_to_str(op));

		case op_plus:
			/* noop */
			return;

		case op_minus:
			out_asm("sub $%s, $%s, $%s",
					r_vtop, sym_regs[MIPS_REG_ZERO], r_vtop);
			break;

		case op_bnot:
			out_asm("not $%s, $%s", r_vtop, r_vtop);
			break;

		case op_not:
			out_asm("seq $%s, $%s, $%s",
					r_vtop, r_vtop, sym_regs[MIPS_REG_ZERO]);
	}
}

void impl_cast_load(type *small, type *big, int is_signed)
{
	if(vtop->type != REG){
		out_comment("// mips cast to %s - loading to register",
				type_to_str(big));
		v_to_reg(vtop);
	}
}

void impl_jmp_reg(int r)
{
	out_asm("jr $%s", reg_str_i(r));
}

void impl_jmp_lbl(const char *lbl)
{
	out_asm("j %s", lbl);
}

void impl_jcond(int true, const char *lbl)
{
	switch(vtop->type){
		case FLAG:
			ICE("FLAG in MIPS");

		case CONST:
			if(true == !!vtop->bits.val){ /* FIXME: factor */
				out_asm("b %s", lbl);
				out_comment("// constant jmp condition %ld", vtop->bits.val);
			}
			break;

		case LBL:
		case STACK:
		case STACK_SAVE:
			v_to_reg(vtop); /* FIXME: factor */

		case REG:
		{
			char buf[REG_STR_SZ];

			reg_str_r(buf, vtop);

			out_asm("beq $%s, $%s, %s", buf, sym_regs[MIPS_REG_ZERO], lbl);
		}
	}
}

void impl_call(const int nargs, type *r_ret, type *r_func)
{
	int i;

	UCC_ASSERT(nargs <= N_CALL_REGS, "TODO: more than 4 args"); /* FIXME */

	for(i = 0; i < nargs; i++){
		const int ri = call_regs[i];

		v_freeup_reg(ri, 1);
		v_to_reg2(vtop, ri);
		vpop();
	}

	switch(vtop->type){
		default:
			v_to_reg(vtop);
		case REG:
			out_asm("jalr $%s", reg_str_i(vtop->bits.reg));
			break;
		case LBL:
			LBL_CHECK(vtop);
			out_asm("jal %s", vtop->bits.lbl.str);
	}
}

void impl_pop_func_ret(type *r)
{
	(void)r;
	v_to_reg2(vtop, REG_RET);
	vpop();
}

void impl_undefined(void)
{
	type *char_ptr = type_new_ptr(
				type_new_type(
					type_new_primitive(type_char)),
				qual_none);

	out_push_zero(char_ptr);
	out_push_zero(char_ptr);
	out_store(); /* *(char *)0 = 0 */
	out_pop();

	out_asm("break"); /* if we're in kernel mode */
}

int impl_frame_ptr_to_reg(int nframes)
{
	ICE("TODO: builtin frame address");
	return -1;
}
