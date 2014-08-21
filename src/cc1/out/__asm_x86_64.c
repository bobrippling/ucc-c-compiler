#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>

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

#include "virt.h" /* v_to* */

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

enum constraint_x86
{
	CONSTRAINT_REG_a = 'a',
	CONSTRAINT_REG_b = 'b',
	CONSTRAINT_REG_c = 'c',
	CONSTRAINT_REG_d = 'd',
	CONSTRAINT_REG_S = 'S',
	CONSTRAINT_REG_D = 'D',
	CONSTRAINT_memory = 'm',
	CONSTRAINT_int = 'n',
	CONSTRAINT_int_asm = 'i',
	CONSTRAINT_REG_any = 'r',
	CONSTRAINT_REG_abcd = 'q',
	CONSTRAINT_REG_float = 'f',
	CONSTRAINT_any = 'g',

	/* modifiers */
	CONSTRAINT_preclobber = '&',
	CONSTRAINT_write_only = '=',
	CONSTRAINT_readwrite = '+'
};

void out_asm_constraint_check(where *w, const char *constraint, int is_output)
{
	const char *const orig = constraint;
	/* currently x86 specific */

	char reg_chosen, mem_chosen, write_only, const_chosen;

	reg_chosen = mem_chosen = write_only = const_chosen = 0;

	while(*constraint){
		int found = 0;
		switch((enum constraint_x86)*constraint++){
			case CONSTRAINT_REG_a:
			case CONSTRAINT_REG_b:
			case CONSTRAINT_REG_c:
			case CONSTRAINT_REG_d:
			case CONSTRAINT_REG_S:
			case CONSTRAINT_REG_D:
			case CONSTRAINT_REG_float:
			case CONSTRAINT_REG_abcd:
			case CONSTRAINT_REG_any:
				reg_chosen++;
				found = 1;
				break;

			case CONSTRAINT_memory:
				mem_chosen = 1;
				found = 1;
				break;

			case CONSTRAINT_int:
			case CONSTRAINT_int_asm:
				const_chosen = 1;
				found = 1;
				break;

			case CONSTRAINT_any:
				found = 1;
				break;

			case CONSTRAINT_write_only:
				write_only = 1;
				found = 1;
				break;

			case CONSTRAINT_preclobber:
				found = 1;
				break;
		}

		if(!found && !isspace(constraint[-1]))
			die_at(w, "invalid constraint character '%c'", constraint[-1]);
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

	if(is_output && reg_chosen + mem_chosen + const_chosen == 0)
		BAD_CONSTRAINT("constraint specifies none of memory/register/const");
}

struct chosen_constraint
{
	enum constraint_type
	{
		C_REG,
		C_MEM,
		C_CONST,
		C_ANY
	} type;

	union
	{
		struct vreg reg;
		enum constraint_const
		{
			CONST_NO,
			CONST_INT,
			CONST_ADDR
		} const_ty;
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
		struct chosen_constraint *constraint,
		const char *str)
{
	int reg = -1, mem = 0, any = 0;
	enum constraint_const const_ty = CONST_NO;

	while(*str){
		int found = 0;

		switch((enum constraint_x86)*str++){
#define CHOOSE(c, i) case c: reg = i; found = 1; break
			CHOOSE(CONSTRAINT_REG_a, X86_64_REG_RAX);
			CHOOSE(CONSTRAINT_REG_b, X86_64_REG_RBX);
			CHOOSE(CONSTRAINT_REG_c, X86_64_REG_RCX);
			CHOOSE(CONSTRAINT_REG_d, X86_64_REG_RDX);
			CHOOSE(CONSTRAINT_REG_S, X86_64_REG_RSI);
			CHOOSE(CONSTRAINT_REG_D, X86_64_REG_RDI);
#undef CHOOSE

			case CONSTRAINT_REG_float:
				ICE("TODO: fp reg constraint");

			case CONSTRAINT_REG_abcd:
				ICE("TODO: a/b/c/d reg");
			case CONSTRAINT_REG_any:
				reg = -1;
				found = 1;
				break;

			case CONSTRAINT_memory:
				found = mem = 1;
				break;
			case CONSTRAINT_int_asm:
				found = 1;
				const_ty = CONST_ADDR;
				break;
			case CONSTRAINT_int:
				found = 1;
				const_ty = CONST_INT;
				break;

			case CONSTRAINT_preclobber:
				found = 1;
				break;

			case CONSTRAINT_write_only:
				found = 1;
				break; /* handled already */

			case CONSTRAINT_any:
				any = found = 1;
				break;
		}

		if(!found && !isspace(str[-1])){
			const char c = str[-1];
			if('0' <= c && c <= '9')
				ICE("TODO: digit/match constraint");
			else
				ICE("TODO: handle constraint '%c'", str[-1]);
		}
	}

	if(any){
		constraint->type = C_ANY;

	}else if(mem){
		constraint->type = C_MEM;

	}else if(const_ty){
		constraint->type = C_CONST;
		constraint->bits.const_ty = const_ty;

	}else{
		constraint->type = C_REG;
		constraint->bits.reg.is_float = 0;
		constraint->bits.reg.idx	= reg;
	}
}

static void constrain_values(
		struct constrained_val *outputs, const size_t noutputs,
		struct chosen_constraint *outputs,
		struct constrained_val *inputs, const size_t ninputs,
		struct chosen_constraint *inputs)
{
	/* pre-scan - if any is just a fixed register we have to allocate it */
	constrain_prescan_fixedreg(
			outputs, oconstraints,
			regs, REG_USED_OUT, 0, error);
	if(error->str) return;

	constrain_prescan_fixedreg(
			inputs, iconstraints,
			regs, REG_USED_IN, outputs->n, error);
	if(error->str) return;

	/* TODO: constrain the rest */
}

static void constrain_val(
		out_ctx *octx,
		struct chosen_constraint *constraint,
		struct constrained_val *cval,
		struct out_asm_error *error)
{
	/* fill it with the right values */
	switch(constraint->type){
		case C_ANY:
			break;

		case C_MEM:
			cval->val = v_to(octx, cval->val, TO_MEM);
			break;

		case C_CONST:
			switch(constraint->bits.const_ty){
				case CONST_NO:
					assert(0);
				case CONST_ADDR:
					/* link-time address or constant int */
					if(cval->val->type == V_LBL)
						break;
					/* fall */
				case CONST_INT:
					if(cval->val->type == V_CONST_I)
						break;
					error->str = ustrdup("can't meet const constraint");
					break;
			}
			break;

		case C_REG:
			if(constraint->bits.reg.idx == (unsigned short)-1){
				/* any reg */
				if(cval->val->type == V_REG){
					/* satisfied */
				}else{
					int got_reg = v_unused_reg(octx, /*spill*/0, /*fp*/0,
							&constraint->bits.reg, cval->val);

					if(!got_reg){
						error->str = ustrprintf(
								"not enough registers to meet constraint \"%s\"",
								cval->constraint);

						error->operand = cval;
					}

					cval->val = v_to_reg_given(
							octx, cval->val, &constraint->bits.reg);
				}
			}else{
				if(cval->val->type != V_REG
				|| cval->val->bits.regoff.reg.idx != constraint->bits.reg.idx)
				{
					const out_val *usedreg = v_find_reg(octx, &constraint->bits.reg);

					if(usedreg){
						error->str = ustrprintf(
								"no free registers for constraint \"%s\"",
								cval->constraint);

						error->operand = cval;
					}

					cval->val = v_to_reg_given_freeup(
							octx, cval->val, &constraint->bits.reg);
				}
			}
			break;
	}
}

static void spill_clobbers(
		out_ctx *octx, char **clobbers, struct out_asm_error *error)
{
	const char *const *regnames = impl_regnames(/*int*/1);

	for(; *clobbers; clobbers++){
		const char *clob = *clobbers;

		if(!strcmp(clob, "memory")){
			/* we don't currently cache memory in registers across
			 * statements - no-op */
		}else if(!strcmp(clob, "cc")){
			/* same for V_FLAG:s */
		}else{
			/* same for registers - just do a validity check on the string */
			const char *const *regi;
			for(regi = regnames; *regi; regi++)
				if(!strcmp(*regi, clob))
					break;

			if(!*regi){
				error->str = ustrprintf(
						"unknown entry in clobber: \"%s\"",
						clob);
				break;
			}
		}
	}
}

void out_inline_asm_extended(
		out_ctx *octx, const char *insn,
		struct constrained_val *outputs, const size_t noutputs,
		struct constrained_val *inputs, const size_t ninputs,
		char **clobbers,
		struct out_asm_error *error)
{
	char *written_insn = NULL;
	size_t insn_len = 0;
	struct
	{
		struct chosen_constraint *inputs, *outputs;
	} constraints;
	const char *p;
	size_t i;

	constraints.inputs = umalloc(ninputs * sizeof *constraints.inputs);
	constraints.outputs = umalloc(noutputs * sizeof *constraints.outputs);

	/* first, spill all the clobber registers,
	 * then we can have a valid list of registers active at asm() time
	 */
	spill_clobbers(octx, clobbers, error);
	if(error->str)
		return;

	constrain_values(
			outputs, noutputs, constraints.outputs,
			inputs, ninputs, constraints.inputs);

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

				constrain_val(octx, constraint, &inputs[this_index], error);

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
	for(i = 0; i < noutputs; i++){
		const struct chosen_constraint *constraint = &constraints.outputs[i];
		const out_val *val = outputs[i].val;

		fprintf(stderr, "found output, index %ld, "
				"constraint %s, exists in TYPE=%d, bits=%d\n",
				(long)i, outputs[i].constraint,
				val->type, val->bits.regoff.reg.idx);

		constrain_output(val, constraint);
	}

	ICW("TODO: free");
}

void out_inline_asm(out_ctx *octx, const char *insn)
{
	out_asm(octx, "%s\n", insn);
}
