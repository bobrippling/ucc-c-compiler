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
#include "../type_nav.h"
#include "../num.h"

#include "out.h" /* our (umbrella) header */

#include "val.h"
#include "impl.h"
#include "asm.h" /* section_type, for write.c: */
#include "write.h"
#include "ctx.h" /* var_stack_sz */

#include "virt.h" /* v_to* */

#include "x86_64.h" /* CC1_IMPL_FNAME */

#include "__asm.h"


#define CONSTRAINT_DEBUG(...)

enum
{
	REG_USED_IN  = 1 << 0,
	REG_USED_OUT = 1 << 1,
};
struct regarray
{
	unsigned char *arr;
	int n;
};

struct constrained_pri_val
{
	struct constrained_val *cval;
	struct chosen_constraint *cchosen;
	int is_output;
	enum
	{
		/* order matters */
		PRIORITY_INT,
		PRIORITY_FIXED_REG,
		PRIORITY_FIXED_CHOOSE_REG,
		PRIORITY_REG,
		PRIORITY_MEM,
		PRIORITY_ANY
	} pri;
};

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
	} bits;
};

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
	CONSTRAINT_REG_D = 'D',
	CONSTRAINT_REG_S = 'S',

	CONSTRAINT_memory = 'm',
	CONSTRAINT_int = 'n',
	CONSTRAINT_int_asm = 'i',
	CONSTRAINT_REG_any = 'r',
	CONSTRAINT_REG_abcd = 'q',
	CONSTRAINT_REG_float = 'f',
	CONSTRAINT_any = 'g',

	/* TODO: o, V, E, F, X
	 * TODO: matching: 0-9 */
};
#define CONSTRAINT_TO_MASK(ch) (1 << ((ch) - 'a' + MODIFIER_COUNT))
#define BITIDX_TO_CONSTRAINT(i) ((i) - MODIFIER_COUNT + 'a')

#define CONSTRAINT_ITER(i) \
	for(i = MODIFIER_COUNT; i < 'z' - 'a' + MODIFIER_COUNT; i++)

enum modifier
{
	MODIFIER_preclobber = '&',
	MODIFIER_write_only = '=',
	MODIFIER_readwrite = '+',
	MODIFIER_COUNT = 3
};
enum modifier_mask
{
	MODIFIER_MASK_preclob = 1 << 0,
	MODIFIER_MASK_write_only = 1 << 1,
	MODIFIER_MASK_rw = 1 << 2
};

void out_asm_calculate_constraint(
		struct constrained_val *cval,
		const char *const constraint,
		const int is_output,
		struct out_asm_error *error)
{
	enum
	{
		RW_UNKNOWN = 0,
		RW_WRITEONLY = 1 << 0,
		RW_READWRITE = 1 << 1,
		RW_PRECLOB = 1 << 2
	} rw = RW_UNKNOWN;
	const char *iter = constraint;
	int has_write;
	unsigned finalmask = 0;

	/* + or = must be first */
	/* & is an early-output, e.g.:
	 *
	 * asm("mov %[input1], %[output1]"
	 *     "mov %[input2], %[output2]"
	 *     : [output1] "=&"(...)
	 *       [output2] "= "(...)
	 *     : [input1]  "xyz"(...)
	 *       [input2]  "xyz"(...))
	 */

	for(; *iter; iter++){
		switch((enum modifier)*iter){
			case MODIFIER_write_only:
				rw |= RW_WRITEONLY;
				finalmask |= MODIFIER_MASK_write_only;
				break;

			case MODIFIER_preclobber:
				/* mark register as being written */
				rw |= RW_PRECLOB;
				finalmask |= MODIFIER_MASK_preclob;
				break;

			case MODIFIER_readwrite:
				/* read and write */
				rw |= RW_READWRITE;
				finalmask |= MODIFIER_MASK_rw;
				break;

			default:
				goto done_mods;
		}
	}
done_mods:;

	for(; *iter; iter++){
		int found = 0;

		switch((enum constraint_x86)*iter){
			case CONSTRAINT_REG_a:
			case CONSTRAINT_REG_b:
			case CONSTRAINT_REG_c:
			case CONSTRAINT_REG_d:
			case CONSTRAINT_REG_S:
			case CONSTRAINT_REG_D:
			case CONSTRAINT_REG_float:
			case CONSTRAINT_REG_abcd:
			case CONSTRAINT_REG_any:
			case CONSTRAINT_memory:
			case CONSTRAINT_int:
			case CONSTRAINT_int_asm:
			case CONSTRAINT_any:
				finalmask |= CONSTRAINT_TO_MASK(*iter);
				found = 1;
				break;
		}
		if(!found && isspace(*iter))
			found = 1;

		if(!found){
			error->operand = cval;
			error->str = ustrprintf("unknown constraint character '%c'", *iter);
			return;
		}
	}

	if(rw & (rw - 1)){
		/* not a power of two - multiple modifiers */
		error->operand = cval;
		error->str = ustrprintf(
				"multiple modifiers for constraint \"%s\"", constraint);
		return;
	}

	has_write = !!(rw & (RW_WRITEONLY | RW_READWRITE));

	if(is_output != has_write){
		error->operand = cval;
		error->str = ustrprintf("%s operand %s %c/%c in constraint",
				is_output ? "output" : "input",
				is_output ? "missing" : "has",
				MODIFIER_write_only, MODIFIER_readwrite);
	}

	if(!is_output && (rw & RW_PRECLOB)){
		error->operand = cval;
		error->str = ustrprintf(
				"constraint modifier '%c' on input",
				MODIFIER_preclobber);
	}

	cval->calculated_constraint = finalmask;
}

static void constrain_output(
		out_ctx *octx,
		const out_val *out_lval,
		const out_val *out_temporary)
{
	out_store(octx, out_lval, out_temporary);
}

static int prioritise_ch(char ch)
{
	switch(ch){
		default:
			return -1;

		case CONSTRAINT_REG_a:
		case CONSTRAINT_REG_b:
		case CONSTRAINT_REG_c:
		case CONSTRAINT_REG_d:
		case CONSTRAINT_REG_D:
		case CONSTRAINT_REG_S:
			return PRIORITY_FIXED_REG;

		case CONSTRAINT_REG_abcd:
			return PRIORITY_FIXED_CHOOSE_REG;

		case CONSTRAINT_memory:
			return PRIORITY_MEM;

		case CONSTRAINT_int:
		case CONSTRAINT_int_asm:
			return PRIORITY_INT;

		case CONSTRAINT_REG_any:
			return PRIORITY_REG;

		case CONSTRAINT_REG_float:
			ICE("TODO: float");

		case CONSTRAINT_any:
			return PRIORITY_ANY;
	}
}

static int prioritise(const unsigned constraint_mask)
{
	int priority = PRIORITY_ANY;
	int i;

	CONSTRAINT_ITER(i){
		if(constraint_mask & (1 << i)){
			int pri = prioritise_ch(BITIDX_TO_CONSTRAINT(i));
			if(pri == -1)
				continue;
			if(pri < priority)
				priority = pri;
		}
	}

	return priority;
}

static int constrained_pri_val_cmp(const void *va, const void *vb)
{
	const struct constrained_pri_val *pa = va;
	const struct constrained_pri_val *pb = vb;

	return pa->pri < pb->pri;
}

static void assign_constraint(
		struct constrained_val *cval,
		struct chosen_constraint *cc,
		const int priority,
		const int is_output,
		struct regarray *regs,
		struct out_asm_error *error)
{
	int regmask = (is_output ? REG_USED_OUT : REG_USED_IN);
	int retry_count;
	unsigned constraint_mask = cval->calculated_constraint;

	if(cval->calculated_constraint & MODIFIER_MASK_preclob)
		regmask |= REG_USED_IN;

	for(retry_count = 0;; retry_count++){
		const int min_priority = priority + retry_count;
		int i;
		int constraint_attempt = -1;

		if(min_priority > PRIORITY_ANY){
			error->operand = cval;
			error->str = ustrprintf(
					"%s constraint unsatisfiable",
					is_output ? "output" : "input");
			return;
		}

		CONSTRAINT_DEBUG("min_priority=%d\n", min_priority);

		/* find the constraint with the lowest priority that we
		 * haven't already tried */
		CONSTRAINT_ITER(i){
			unsigned mask = 1 << i;

			if(constraint_mask & mask){
				const int ch = BITIDX_TO_CONSTRAINT(i);
				const int this_pri = prioritise_ch(ch);

				CONSTRAINT_DEBUG("  constraint mask has '%c'\n", ch);

				if(this_pri <= min_priority){
					/* found one, try it */
					constraint_attempt = ch;

					/* don't try again later */
					constraint_mask &= ~mask;

					CONSTRAINT_DEBUG("    found %c (priority %d)\n",
							ch, min_priority);
					break;
				}
			}
		}

		if(constraint_attempt == -1)
			continue;

		CONSTRAINT_DEBUG("trying constraint '%c'\n", constraint_attempt);

		switch(constraint_attempt){
				int chosen_reg;

			case CONSTRAINT_REG_a: chosen_reg = X86_64_REG_RAX; goto reg;
			case CONSTRAINT_REG_b: chosen_reg = X86_64_REG_RBX; goto reg;
			case CONSTRAINT_REG_c: chosen_reg = X86_64_REG_RCX; goto reg;
			case CONSTRAINT_REG_d: chosen_reg = X86_64_REG_RDX; goto reg;
			case CONSTRAINT_REG_D: chosen_reg = X86_64_REG_RDI; goto reg;
			case CONSTRAINT_REG_S: chosen_reg = X86_64_REG_RSI; goto reg;
	reg:
			{
				if(regs->arr[chosen_reg] & regmask)
					continue; /* try again */

				regs->arr[chosen_reg] |= regmask;
				cc->type = C_REG;
				cc->bits.reg.idx = chosen_reg;
				cc->bits.reg.is_float = 0;
				break;
			}

			case CONSTRAINT_REG_any:
			case CONSTRAINT_REG_abcd:
			{
				const int lim = (constraint_attempt == CONSTRAINT_REG_abcd
						? X86_64_REG_RDX + 1
						: regs->n);
				int i;

				/* pick the first one - prioritised so this is fine */
				for(i = 0; i < lim; i++){
					if((regs->arr[i] & regmask) == 0){
						regs->arr[i] |= regmask;
						cc->type = C_REG;
						cc->bits.reg.idx = i;
						cc->bits.reg.is_float = 0;
						break;
					}
				}

				if(i == lim)
					continue; /* try again */
				break;
			}

			case CONSTRAINT_memory:
				cc->type = C_MEM;
				break;

			case CONSTRAINT_int_asm:
				/* link-time address or constant int */
				switch(cval->val->type){
					case V_LBL:
					case V_CONST_I:
						break; /* fall */
					default:
						continue; /* try again */
				}
				if(0){
			case CONSTRAINT_int:
					if(cval->val->type != V_CONST_I)
						continue; /* try again */
				}
				cc->type = C_CONST;
				break;

			case CONSTRAINT_REG_float:
				ICE("TODO: float");

			case CONSTRAINT_any:
				cc->type = C_ANY;
				break;
		}

		/* constraint met */
		break;
	}
}

static void assign_constraints(
		struct constrained_pri_val *entries, size_t nentries,
		struct regarray *regs, struct out_asm_error *error)
{
	size_t i;
	for(i = 0; i < nentries; i++){
		struct constrained_val *cval = entries[i].cval;
		struct chosen_constraint *cc = entries[i].cchosen;

		/* pick the highest satisfiable constraint */
		assign_constraint(cval, cc,
				entries[i].pri, entries[i].is_output,
				regs, error);

		if(error->str)
			break;
	}
}

static void calculate_constraints(
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs,
		struct chosen_constraint *oconstraints,
		struct chosen_constraint *iconstraints,
		struct regarray *regs,
		struct out_asm_error *error)
{
	size_t const nentries = outputs->n + inputs->n;
	struct constrained_pri_val *entries;
	size_t i;

	entries = umalloc(nentries * sizeof *entries);
	for(i = 0; i < nentries; i++){
		struct constrained_pri_val *p = &entries[i];
		struct constrained_val *from_val;
		struct chosen_constraint *from_cc;

		if(i >= outputs->n){
			from_val = &inputs->arr[i - outputs->n];
			from_cc = &iconstraints[i - outputs->n];
			p->is_output = 0;
		}else{
			from_val = &outputs->arr[i];
			from_cc = &oconstraints[i];
			p->is_output = 1;
		}

		p->cval = from_val;
		p->cchosen = from_cc;
		p->pri = prioritise(from_val->calculated_constraint);
	}

	/* order them */
	qsort(entries, nentries, sizeof *entries, constrained_pri_val_cmp);

	/* assign values */
	assign_constraints(entries, nentries, regs, error);

	free(entries);
}

static int asm_get_reg(
		out_ctx *octx,
		struct vreg *regp, const out_val *from,
		struct out_asm_error *error)
{
	int got_reg = v_unused_reg(octx, /*spill*/0, /*fp*/0, regp, from);

	if(!got_reg)
		error->str = ustrdup("not enough registers to meet constraint");

	return !got_reg;
}

static void asm_try_getreg(
		out_ctx *octx, struct vreg const *regp,
		struct out_asm_error *error)
{
	const out_val *usedreg = v_find_reg(octx, regp);

	if(usedreg)
		error->str = ustrdup("register already in use");
}

static void constrain_input_val(
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
			break;

		case C_REG:
			if(constraint->bits.reg.idx == (unsigned short)-1){
				/* any reg */
				if(cval->val->type == V_REG){
					/* satisfied */
				}else{
					if(asm_get_reg(octx, &constraint->bits.reg, cval->val, error))
						return;

					cval->val = v_to_reg_given(
							octx, cval->val, &constraint->bits.reg);
				}
			}else{
				if(cval->val->type != V_REG
				|| cval->val->bits.regoff.reg.idx != constraint->bits.reg.idx)
				{
					asm_try_getreg(octx, &constraint->bits.reg, error);

					if(error->str) return;

					cval->val = v_to_reg_given_freeup(
							octx, cval->val, &constraint->bits.reg);
				}
			}
			break;
	}
}

static const out_val *temporary_for_output(
		out_ctx *octx,
		struct chosen_constraint *constraint,
		struct constrained_val *cval,
		struct out_asm_error *error)
{
	/* if the output already references the lvalue and matches the constraint,
	 * we can leave it.
	 *
	 * otherwise we construct an out_val that matches the constraint and use
	 * that as a temporary, then assign to the output afterwards
	 */
	switch(constraint->type){
		case C_ANY:
			return NULL;

		case C_REG:
			if(constraint->bits.reg.idx == (unsigned short)-1){
				struct vreg reg;

				if(cval->val->type == V_REG)
					return NULL; /* matched */

				if(asm_get_reg(octx, &reg, NULL, error)){
					/* error set */
					return NULL;
				}

				return v_new_reg(octx, NULL,
						type_dereference_decay(cval->val->t),
						&reg);

			}else{
				if(cval->val->type == V_REG
				&& vreg_eq(&cval->val->bits.regoff.reg, &constraint->bits.reg))
				{
					return NULL; /* matched */
				}

				return v_new_reg(octx, NULL,
						type_dereference_decay(cval->val->t),
						&constraint->bits.reg);
			}
			assert(0);

		case C_MEM:
		{
			out_val *mutreg;
			long stack_off;

			switch(cval->val->type){
				case V_REG:
					break;
				case V_LBL:
				case V_REG_SPILT:
					return NULL; /* matched */
				default:
					break;
			}

			v_alloc_stack(octx, type_size(cval->val->t, NULL), "asm output temporary");
			stack_off = octx->var_stack_sz;

			mutreg = v_new_bp3_below(octx, NULL,
					type_dereference_decay(cval->val->t),
					stack_off);

			mutreg->type = V_REG_SPILT;

			return mutreg;
		}

		case C_CONST:
			if(cval->val->type != V_CONST_I){
				error->str = ustrdup("operand not a constant");
				error->operand = cval;
			}
			return NULL;
	}

	assert(0);
}

static void parse_clobbers(
		char **clobbers,
		struct regarray *const regs,
		struct out_asm_error *error)
{
	if(!clobbers)
		return;

	for(; *clobbers; clobbers++){
		const char *clob = *clobbers;

		if(!strcmp(clob, "memory")){
			/* we don't currently cache memory in registers across
			 * statements - no-op */
		}else if(!strcmp(clob, "cc")){
			/* same for V_FLAG:s */
		}else{
			/* same for registers - just do a validity check on the string */
			int regi = impl_regname_index(clob);

			if(regi == -1){
				error->str = ustrprintf(
						"unknown entry in clobber: \"%s\"",
						clob);
				break;
			}

			regs->arr[regi] |= REG_USED_IN | REG_USED_OUT;
		}
	}
}

void out_asm_release_valarray(
		out_ctx *octx, struct constrained_val_array *vals)
{
	size_t i;
	for(i = 0; i < vals->n; i++)
		out_val_release(octx, vals->arr[i].val);
}

static void constrain_values(out_ctx *octx,
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs,
		struct chosen_constraint *coutputs,
		struct chosen_constraint *cinputs,
		const out_val *output_temporaries[],
		struct out_asm_error *error)
{
	size_t const total = outputs->n + inputs->n;
	size_t i;

	for(i = 0; i < total; i++){
		if(i >= outputs->n){
			struct chosen_constraint *constraint;
			size_t input_i = i - outputs->n;

			assert(input_i < inputs->n);

			/* get this input into the memory/register/constant
			 * for the asm. if we can't, hard error */
			constraint = &cinputs[input_i];

			constrain_input_val(octx, constraint, &inputs->arr[input_i], error);

			if(error->str){
				error->operand = &inputs->arr[input_i];
				return;
			}

		}else{
			const out_val *out_temporary;

			/* attempt to get the lvalue referenced by 'output'
			 * into a memory/register/constant for this constraint.
			 * if not, we move it there afterwards */
			out_temporary = temporary_for_output(
					octx,
					&coutputs[i],
					&outputs->arr[i],
					error);

			if(error->str){
				error->operand = &outputs->arr[i];
				return;
			}

			output_temporaries[i] = out_temporary;
			/* TODO: if this is a '+' / readwrite, dereference it beforehand? */
		}
	}
}

static char *format_insn(const char *format,
		struct constrained_val_array *outputs,
		const out_val *output_temporaries[],
		struct constrained_val_array *inputs)
{
	char *written_insn = NULL;
	size_t insn_len = 0;
	const char *p;

	for(p = format; *p; p++){
		if(*p == '%' && *++p != '%'){
			size_t this_index;
			char *end;
			const out_val *oval;

			if(*p == '['){
				ICE("TODO: named constraint");
			}

			this_index = strtol(p, &end, 0);
			if(end == p)
				ICE("not an int - should've been caught");
			p = end - 1; /* ready for ++ */

			if(this_index >= outputs->n){
				this_index -= outputs->n;

				if(this_index > inputs->n)
					ICE("index %lu oob", (unsigned long)this_index);

				oval = inputs->arr[this_index].val;
			}else{
				if((oval = output_temporaries[this_index]))
					;
				else
					oval = outputs->arr[this_index].val;
			}


			{
				const char *val_str;
				char *op;
				size_t oplen;
				int deref;

				deref = (oval->type == V_REG_SPILT);

				val_str = impl_val_str(oval, deref);
				oplen = strlen(val_str);

				op = dynvec_add_n(&written_insn, &insn_len, oplen);

				memcpy(op, val_str, oplen);
			}
		}else{
			*(char *)dynvec_add(&written_insn, &insn_len) = *p;
		}
	}

	*(char *)dynvec_add(&written_insn, &insn_len) = '\0';

	return written_insn;
}

void out_inline_asm_extended(
		out_ctx *octx, const char *format,
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs,
		char **clobbers,
		struct out_asm_error *error)
{
	struct
	{
		struct chosen_constraint *inputs, *outputs;
	} constraints;
	struct regarray regs;
	const out_val **output_temporaries;
	char *insn;
	size_t i;

	constraints.inputs = umalloc(inputs->n * sizeof *constraints.inputs);
	constraints.outputs = umalloc(outputs->n * sizeof *constraints.outputs);

	output_temporaries = umalloc(outputs->n * sizeof *output_temporaries);

	regs.arr = v_alloc_reg_reserve(octx, &regs.n);

	/* first, spill all the clobber registers,
	 * then we can have a valid list of registers active at asm() time
	 */
	parse_clobbers(clobbers, &regs, error);
	if(error->str) goto error;

	calculate_constraints(
			outputs, inputs,
			constraints.outputs, constraints.inputs,
			&regs, error);
	if(error->str) goto error;

	constrain_values(octx,
			outputs, inputs,
			constraints.outputs,
			constraints.inputs,
			output_temporaries,
			error);
	if(error->str) goto error;

	insn = format_insn(format, outputs, output_temporaries, inputs);

	out_comment(octx, "### actual inline");
	out_asm(octx, "%s", insn ? insn : "");
	out_comment(octx, "### assignments to outputs");

	/* consume inputs */
	out_asm_release_valarray(octx, inputs);

	free(insn), insn = NULL;

	/* store to the output pointers */
	for(i = 0; i < outputs->n; i++){
		const out_val *val = outputs->arr[i].val;

		if(output_temporaries[i])
			constrain_output(octx, val, output_temporaries[i]);
		else
			out_val_release(octx, val);
	}

	if(0){
error:
		/* release temporaries */
		for(i = 0; i < outputs->n; i++)
			if(output_temporaries[i])
				out_val_release(octx, output_temporaries[i]);
	}

	free(output_temporaries);
	free(regs.arr);
	free(constraints.inputs);
	free(constraints.outputs);
}

void out_inline_asm(out_ctx *octx, const char *insn)
{
	out_asm(octx, "%s\n", insn);
}
