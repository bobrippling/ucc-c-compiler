#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "../../util/util.h"
#include "../../util/dynarray.h"
#include "../../util/dynvec.h"
#include "../../util/dynmap.h"
#include "../../util/alloc.h"

#include "../type.h"
#include "../num.h"

#include "out.h" /* our (umbrella) header */

#include "val.h"
#include "impl.h"
#include "asm.h" /* section_type, for write.c: */
#include "write.h"

#include "x86_64.h" /* CC1_IMPL_FNAME */

#include "__asm.h"


#if 0
+---+--------------------+
| r |    Register(s)     |
+---+--------------------+
| a |   %eax, %ax, %al   |
| b |   %ebx, %bx, %bl   |
| c |   %ecx, %cx, %cl   |
| d |   %edx, %dx, %dl   |
| S |   %esi, %si        |
| D |   %edi, %di        |
+---+--------------------+

  m |   memory
  i |   integral
  r |   any reg
  q |   reg [abcd]
  f |   fp reg
  & |   pre-clobber


  = | write-only - needed in output

http://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html#s4

#endif

enum
{
	CONSTRAINT_REG_a = 'a',
	CONSTRAINT_REG_b = 'b',
	CONSTRAINT_REG_c = 'c',
	CONSTRAINT_REG_d = 'd',
	CONSTRAINT_REG_S = 'S',
	CONSTRAINT_REG_D = 'D',
	CONSTRAINT_memory = 'm',
	CONSTRAINT_int = 'i',
	CONSTRAINT_REG_any = 'r',
	CONSTRAINT_REG_abcd = 'q',
	CONSTRAINT_REG_float = 'f',
	CONSTRAINT_preclobber = '&',
	CONSTRAINT_write_only = '='
};

void out_asm_constraint_check(where *w, const char *constraint, int is_output)
{
	const char *const orig = constraint;
	/* currently x86 specific */
	size_t first_not = strspn(constraint, "gabcdSDmirqf&=");

	if(first_not != strlen(constraint))
		die_at(w, "invalid constraint \"%c\"", constraint[first_not]);

	{
		char reg_chosen, mem_chosen, write_only, const_chosen;

		reg_chosen = mem_chosen = write_only = const_chosen = 0;

		while(*constraint)
			switch(*constraint++){
				case 'a':
				case 'b':
				case 'c':
				case 'd':
				case 'S':
				case 'D':
				case 'f':
				case 'q':
				case 'r':
					reg_chosen++;
					break;

				case 'm':
					mem_chosen = 1;
					break;

				case 'n':
					const_chosen = 1;
					break;

				case 'g':
					/* any */
					ICE("TODO: any constraint");
					break;

				case '=':
					write_only = 1;
					break;
			}

#define BAD_CONSTRAINT(err) \
		die_at(w, "bad constraint \"%s\": " err, orig)

		if(is_output != write_only)
			die_at(w, "%s output constraint", is_output ? "missing" : "unwanted");

		if(is_output && const_chosen)
			BAD_CONSTRAINT("can't output to a constant");

		/* TODO below: allow multiple options for a constraint */
		if(reg_chosen > 1)
			BAD_CONSTRAINT("too many registers");

		switch(reg_chosen + mem_chosen + const_chosen){
			case 0:
				if(is_output)
					BAD_CONSTRAINT("constraint specifies none of memory/register/const");
				/* fall */

			case 1:
				/* fine - exactly one */
				break;

			default:
				BAD_CONSTRAINT("constraint specifies memory/register/const combination");
		}
	}
}

struct chosen_constraint
{
	enum constraint_type
	{
		C_REG,
		C_MEM,
		C_CONST,
	} type;

	union
	{
		struct vreg reg;
		unsigned long stack;
		unsigned long k;
	} bits;
};

static void constrain_output(
		const out_val *val, const struct chosen_constraint *cc)
{
#if 0
	/* pop from register/memory to vp */
	struct vstack *vp = &vtop[-lval_vp_offset];

	ICW("asm() storing to output doesn't work");

	switch(cc->type){
		case C_MEM:
			switch(vp->type){
				/* only certain ones are valid,
				 * may need mem->reg, reg->mem, etc */
				case V_CONST_I:
				case V_CONST_F:
				case V_FLAG:
					goto bad;

				case V_REG_SAVE:
				case V_LBL:
				case V_REG:
					/* create a mem-pointer vstack to assign to */
					vpush(type_ref_ptr_depth_inc(vp->t));

					/* FIXME: what about labels? */
					v_set_stack(vtop, NULL, cc->bits.stack, 1);

					/* vstack is:
					 * vp  { type = REG }
					 * ptr { type = STACK }
					 */
					out_store();
			}
			break;

		case C_REG:
			switch(vp->type){
				/* only certain ones are valid,
				 * may need mem->reg, reg->mem, etc */
				case V_CONST_I:
				case V_CONST_F:
				case V_FLAG:
					goto bad;

				case V_REG_SAVE:
				case V_LBL:
					/* FIXME: this doesn't work */
					vpush(vp->t);

					v_set_stack(vtop, NULL, cc->bits.stack, 1);
					/* FIXME: also what about labels? */

					impl_store(vp, vtop);
					out_pop();
					break;

				case V_REG:
					ICE("TODO");
					/*impl_reg_cp_rev(vp, cc->bits.reg);*/
			}
			break;

		case C_CONST:
bad:
			ICE("bad unconstraint type C_CONST");
	}
#endif
}

static void populate_constraint(
		const char *constraint, struct chosen_constraint *con)
{
#if 0
	int reg = -1, mem = 0, is_const = 0;

	while(*constraint)
		switch(*constraint++){
#define CHOOSE(c, i) case c: reg = i; break
			CHOOSE('a', X86_64_REG_RAX);
			CHOOSE('b', X86_64_REG_RBX);
			CHOOSE('c', X86_64_REG_RCX);
			CHOOSE('d', X86_64_REG_RDX);
#undef CHOOSE

			case 'f': ICE("TODO: fp reg constraint");

			case 'S': reg = X86_64_REG_RSI; break;
			case 'D': reg = X86_64_REG_RDI; break;

			case 'q': /* currently the same as 'r' */
			case 'r':
				reg = -1;
				break;

			case 'm':
				mem = 1;
				break;
			case 'n':
				is_const = 1;
				break;

			default:
			{
				const char c = constraint[-1];
				if('0' <= c && c <= '9')
					ICE("TODO: digit/match constraint");
			}
		}


	if(mem){
		con->type = C_MEM;

	}else if(is_const){
		con->type = C_CONST;

	}else{
		con->type = C_REG;
		con->bits.reg.is_float = 0;
		con->bits.reg.idx	= reg;
	}
#endif
}

static void constrain_oval(
		struct chosen_constraint *constraint,
		struct constrained_val *cval)
{
#if 0
	/* pick one */
	populate_constraint(io->constraints, cc);

	if(already_set){
		if(!CONSTRAINT_EQ_OUT_STORE(cc->type, vp->type))
			die_at(err_w, "couldn't satisfy mismatching constraints");

	}else{
		/* fill it with the right values */
		switch(cc->type){
			case C_MEM:
				/* vp into memory */
				v_to_mem(vp);
				cc->bits.stack = vp->bits.regoff.offset;
				break;

			case C_CONST:
				if(vp->type != V_CONST_I)
					die_at(&io->exp->where, "invalid operand for const-constraint");
				cc->bits.k = vp->bits.val_i;
				break;

			case C_REG:
				if(cc->bits.reg.idx == (short unsigned)-1){
					if(vp->type == V_REG){
						cc->bits.reg = vp->bits.regoff.reg;
					}else{
						v_unused_reg(1, 0, &cc->bits.reg);

						v_freeup_reg(&cc->bits.reg, 1);
						v_to_reg(vp); /* TODO: v_to_reg_preferred */
					}
				}

				if(vp->bits.regoff.reg.idx != cc->bits.reg.idx){
					impl_reg_cp(vp, &cc->bits.reg);
					vp->bits.regoff.reg = cc->bits.reg;
				}
				break;
		}
	}
#endif
}

void out_inline_asm_extended(
		out_ctx *octx, const char *insn,
		struct constrained_val *outputs, const size_t noutputs,
		struct constrained_val *inputs, const size_t ninputs,
		char **clobbers)
{
	char *written_insn = NULL;
	size_t insn_len = 0;
	struct
	{
		struct chosen_constraint *inputs, *outputs;
	} constraints;
	const char *p;

	constraints.inputs = umalloc(ninputs * sizeof *constraints.inputs);
	constraints.outputs = umalloc(noutputs * sizeof *constraints.outputs);

	for(p = insn; *p; p++){
		if(*p == '%' && *++p != '%'){
			char *end;
			size_t this_index;

			if(*p == '['){
				ICE("TODO: named constraint");
			}

			this_index = strtol(p, &end, 0);
			if(end == p)
				ICE("not an int - should've been caught");
			p = end - 1; /* ready for ++ */

			if(this_index >= noutputs){
				struct chosen_constraint *constraint;
				const char *val_str;
				char *op;
				size_t oplen;

				this_index -= noutputs;
				assert(this_index < ninputs);

				/* get this input into the memory/register/constant
				 * for the asm. if we can't, hard error */
				constraint = &constraints.inputs[this_index];

				constrain_val(constraint, &inputs[this_index]);

				val_str = impl_val_str(inputs[this_index].val, /*deref*/0);
				oplen = strlen(val_str);

				op = dynvec_add_n(&written_insn, &insn_len, oplen);

				memcpy(op, val_str, oplen);

			}else{
				const out_val *output = outputs[this_index].val;

				/* attempt to get the lvalue referenced by 'output'
				 * into a memory/register/constant for this constraint.
				 * if not, we move it there afterwards */

				/* XXX: TODO */
				ICE("TODO: output");
				/*
				out_constrain(
						io, vp,
						&constraints[this_index],
						constraint_set[this_index]++,
						err_w);
				*/
			}

		}else{
			*(char *)dynvec_add(&written_insn, &insn_len) = *p;
		}
	}

	*(char *)dynvec_add(&written_insn, &insn_len) = '\0';

	out_comment(octx, "### actual inline");
	out_asm(octx, "%s", written_insn ? written_insn : "");
	out_comment(octx, "### assignments to outputs");

	/* consume inputs */
	for(i = 0; i < ninputs; i++)
		out_val_release(octx, inputs[i].val);

	free(written_insn), written_insn = NULL;

	/* store to the output pointers */
	{
		size_t i;
		for(i = 0; i < noutputs; i++){
			const struct chosen_constraint *constraint = &constraints.outputs[i];
			const out_val *val = outputs[i].val;

			fprintf(stderr, "found output, index %ld, "
					"constraint %s, exists in TYPE=%d, bits=%d\n",
					(long)i, outputs[i].constraint,
					val->type, val->bits.regoff.reg.idx);

			constrain_output(val, constraint);
		}
	}

	ICW("TODO: free");
}

void out_inline_asm(out_ctx *octx, const char *insn)
{
	out_asm(octx, "%s\n", insn);
}
