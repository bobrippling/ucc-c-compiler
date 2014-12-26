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
#include "../str.h" /* str_add_escape() */
#include "../cc1.h" /* fopt_mode */

#include "out.h" /* our (umbrella) header */

#include "val.h"
#include "impl.h"
#include "asm.h" /* section_type, for write.c: */
#include "write.h"
#include "ctx.h" /* var_stack_sz */
#include "macros.h"
#include "lbl.h"

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
		C_TO_REG_OR_MEM,
		C_MATCH
	} type;

	union
	{
		struct vreg reg;
		struct
		{
			struct constrained_val *cval;
			struct chosen_constraint *constraint;
		} match;
	} bits;
};

struct asm_setup_state
{
	out_ctx *octx;
	asm_callback_fn *gen_callback;
	void *gen_callback_ctx;
	struct regarray *regs;
	type *fnty;
	struct out_asm_error *error;
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

	CONSTRAINT_0_to_31 = 'I',
	CONSTRAINT_0_to_63 = 'J',
	CONSTRAINT_8bit_signed = 'K',
	CONSTRAINT_0123 = 'M',
	CONSTRAINT_8bit_unsigned = 'N',

	CONSTRAINT_0 = '0', CONSTRAINT_1 = '1', CONSTRAINT_2 = '2',
	CONSTRAINT_3 = '3', CONSTRAINT_4 = '4', CONSTRAINT_5 = '5',
	CONSTRAINT_6 = '6', CONSTRAINT_7 = '7', CONSTRAINT_8 = '8',
	CONSTRAINT_9 = '9',

	/* TODO: o, V, E, F, X
	 * TODO: matching: 0-9 */
};
enum constraint_mask
{
	/* start after modifier mask */
	CONSTRAINT_MASK_REG_a = 1 << 3,
	CONSTRAINT_MASK_REG_b = 1 << 4,
	CONSTRAINT_MASK_REG_c = 1 << 5,
	CONSTRAINT_MASK_REG_d = 1 << 6,
	CONSTRAINT_MASK_REG_D = 1 << 7,
	CONSTRAINT_MASK_REG_S = 1 << 8,

	CONSTRAINT_MASK_memory = 1 << 9,
	CONSTRAINT_MASK_int = 1 << 10,
	CONSTRAINT_MASK_int_asm = 1 << 11,
	CONSTRAINT_MASK_REG_any = 1 << 12,
	CONSTRAINT_MASK_REG_abcd = 1 << 13,
	CONSTRAINT_MASK_REG_float = 1 << 14,

	CONSTRAINT_MASK_0_to_31 = 1 << 15,
	CONSTRAINT_MASK_0_to_63 = 1 << 16,
	CONSTRAINT_MASK_8bit_signed = 1 << 17,
	CONSTRAINT_MASK_0123 = 1 << 18,
	CONSTRAINT_MASK_8bit_unsigned = 1 << 19,

	CONSTRAINT_MASK_any = 1 << 20,

	/* 9 = 1001, 4 bits to encode a match-constraint.
	 * match-constraint encoded as match+1, since we
	 * can't tell if 0 is present
	 *
	 * 1<<21 to 1<<25 are reserved */
#define MATCHING_CONSTRAINT_SHIFT 21
#define MATCHING_CONSTRAINT_MASK  15 /* 1111 */
#define MATCHING_CONSTRAINT_MASK_SHIFTED (MATCHING_CONSTRAINT_MASK << MATCHING_CONSTRAINT_SHIFT)

#define CONSTRAINT_MAX_BIT (20 + 4) /* include matching bits */
};

#define CONSTRAINT_ITER(i) \
	for(i = MODIFIER_COUNT; i <= CONSTRAINT_MAX_BIT; i++)

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

unsigned out_asm_calculate_constraint(
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
	enum constraint_mask finalmask = 0;

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
#define MAP(post)                              \
			case CONSTRAINT_ ## post:                \
				found = 1;                             \
				finalmask |= CONSTRAINT_MASK_ ## post; \
				break
			MAP(REG_a);
			MAP(REG_b);
			MAP(REG_c);
			MAP(REG_d);
			MAP(REG_S);
			MAP(REG_D);
			MAP(REG_float);
			MAP(REG_abcd);
			MAP(REG_any);
			MAP(memory);
			MAP(int);
			MAP(int_asm);
			MAP(0_to_31);
			MAP(0_to_63);
			MAP(8bit_signed);
			MAP(0123);
			MAP(8bit_unsigned);
			MAP(any);
#undef MAP

			case '0': case '1': case '2':
			case '3': case '4': case '5':
			case '6': case '7': case '8':
			case '9':
				found = 1;
				if(finalmask & MATCHING_CONSTRAINT_MASK_SHIFTED){
					error->str = ustrprintf(
							"can't handle two matching constraints (%c)",
							*iter);
					return 0;
				}
				finalmask |= (*iter - '0' + 1) << MATCHING_CONSTRAINT_SHIFT;
				break;
		}
		if(!found && isspace(*iter))
			found = 1;

		if(!found){
			switch(*iter){
				case MODIFIER_preclobber:
				case MODIFIER_write_only:
				case MODIFIER_readwrite:
					error->str = ustrprintf(
							"modifier character not at start ('%c')",
							*iter);
					break;
				default:
					error->str = ustrprintf("unknown constraint character '%c'", *iter);
			}
			return 0;
		}
	}

	if(is_output
	&& finalmask & (CONSTRAINT_MASK_int | CONSTRAINT_MASK_int_asm))
	{
		error->str = ustrdup("'i' constraint in output");
		return 0;
	}

	if((rw & (RW_WRITEONLY | RW_READWRITE)) == (RW_WRITEONLY | RW_READWRITE)){
		error->str = ustrprintf(
				"writeonly and readwrite modifiers for constraint \"%s\"",
				constraint);
		return 0;
	}

	has_write = !!(rw & (RW_WRITEONLY | RW_READWRITE));

	if(is_output != has_write){
		error->str = ustrprintf("%s operand %s %c/%c in constraint",
				is_output ? "output" : "input",
				is_output ? "missing" : "has",
				MODIFIER_write_only, MODIFIER_readwrite);
	}

	if(!is_output && (rw & RW_PRECLOB)){
		error->str = ustrprintf(
				"constraint modifier '%c' on input",
				MODIFIER_preclobber);
	}

	return finalmask;
}

static void constrain_output(
		out_ctx *octx,
		const out_val *out_lval,
		const out_val *out_temporary,
		const struct chosen_constraint *constraint)
{
	int temporary_is_lvalue = (constraint->type == C_MEM);

	if(temporary_is_lvalue)
		out_temporary = out_deref(octx, out_temporary);

	out_store(octx, out_lval, out_temporary);
}

static int prioritise_mask(enum constraint_mask mask)
{
	switch(mask){
		case CONSTRAINT_MASK_REG_a:
		case CONSTRAINT_MASK_REG_b:
		case CONSTRAINT_MASK_REG_c:
		case CONSTRAINT_MASK_REG_d:
		case CONSTRAINT_MASK_REG_D:
		case CONSTRAINT_MASK_REG_S:
			return PRIORITY_FIXED_REG;

		case CONSTRAINT_MASK_REG_abcd:
			return PRIORITY_FIXED_CHOOSE_REG;

		case CONSTRAINT_MASK_memory:
			return PRIORITY_MEM;

		case CONSTRAINT_MASK_0_to_31:
		case CONSTRAINT_MASK_0_to_63:
		case CONSTRAINT_MASK_8bit_signed:
		case CONSTRAINT_MASK_0123:
		case CONSTRAINT_MASK_8bit_unsigned:
		case CONSTRAINT_MASK_int:
		case CONSTRAINT_MASK_int_asm:
			return PRIORITY_INT;

		case CONSTRAINT_MASK_REG_any:
			return PRIORITY_REG;

		case CONSTRAINT_MASK_REG_float:
			ICE("TODO: float");

		case CONSTRAINT_MASK_any:
			return PRIORITY_ANY;
	}

	return -1;
}

static int prioritise(const enum constraint_mask in_mask)
{
	int priority = PRIORITY_ANY;
	int i;

	CONSTRAINT_ITER(i){
		enum constraint_mask onebit = 1 << i;
		if(in_mask & onebit){
			int pri = prioritise_mask(onebit);
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

	/* we want 'a' before 'b', smallest first */
	return pa->pri - pb->pri;
}

static int assign_constraint_pick_reg(
		struct chosen_constraint *cc,
		struct regarray *regs,
		const unsigned regmask,
		const int lim,
		type *fnty)
{
	int include_callee_save = 0;
	int i;
retry_reg:
	for(i = 0; i < lim; i++){
		if((regs->arr[i] & regmask) == 0){

			cc->bits.reg.idx = i;
			cc->bits.reg.is_float = 0;

			if(!include_callee_save
			&& impl_reg_is_callee_save(fnty, &cc->bits.reg))
			{
				continue;
			}

			regs->arr[i] |= regmask;
			cc->type = C_REG;
			break;
		}
	}

	if(i == lim){
		if(!include_callee_save){
			include_callee_save = 1;
			goto retry_reg;
		}
		return 0;
	}
	return 1;
}

static void callback_gen_val(
		struct asm_setup_state *setupstate,
		struct constrained_val *cval)
{
	if(cval->val)
		return;

	if(fopt_mode & FOPT_VERBOSE_ASM)
		out_comment(setupstate->octx, "callback_gen_val()");

	setupstate->gen_callback(
			setupstate->octx, cval, setupstate->gen_callback_ctx);

	assert(cval->val && "no value generated");
}

static int valid_int_constraint(
		enum constraint_mask constraint, const out_val *val)
{
	integral_t i, min, max;

	if(val->type != V_CONST_I)
		return 0;

	switch(constraint){
		default:
			assert(0 && "non-int constraint");

		case CONSTRAINT_MASK_int:
			return 1;

		case CONSTRAINT_MASK_0_to_31:       min = 0,    max = 31; break;
		case CONSTRAINT_MASK_0_to_63:       min = 0,    max = 31; break;
		case CONSTRAINT_MASK_8bit_signed:   min = -128, max = 127; break;
		case CONSTRAINT_MASK_0123:          min = 0,    max = 3; break;
		case CONSTRAINT_MASK_8bit_unsigned: min = 0,    max = 256; break;
	}

	i = val->bits.val_i;

	return min <= i && i <= max;
}

static void assign_constraint(
		struct asm_setup_state *setupstate,
		struct constrained_pri_val *const entries,
		const size_t this_entry_i)
{
	struct constrained_val *const cval = entries[this_entry_i].cval;
	struct chosen_constraint *const cc = entries[this_entry_i].cchosen;
	const int priority = entries[this_entry_i].pri;
	const int is_output = entries[this_entry_i].is_output;

	struct regarray *const regs = setupstate->regs;
	int regmask = (is_output ? REG_USED_OUT : REG_USED_IN);
	int retry_count;
	enum constraint_mask whole_constraint = cval->calculated_constraint;

	if(cval->calculated_constraint & (MODIFIER_MASK_preclob | MODIFIER_MASK_rw))
		regmask |= REG_USED_IN;

	for(retry_count = 0;; retry_count++){
		const int min_priority = priority + retry_count;
		unsigned constraint_i;
		enum constraint_mask constraint_attempt = -1;

		if(min_priority > PRIORITY_ANY){
			setupstate->error->operand = cval;
			setupstate->error->str = ustrprintf(
					"%s constraint unsatisfiable",
					is_output ? "output" : "input");
			return;
		}

		CONSTRAINT_DEBUG("min_priority=%d\n", min_priority);

		/* find the constraint with the lowest priority that we
		 * haven't already tried */
		CONSTRAINT_ITER(constraint_i){
			enum constraint_mask onebit = 1 << constraint_i;

			if(whole_constraint & onebit){
				const int this_pri = prioritise_mask(onebit);

				if(this_pri <= min_priority){
					/* found one, try it */
					constraint_attempt = onebit;

					/* don't try again later */
					whole_constraint &= ~onebit;
					break;
				}
			}
		}

		if((int)constraint_attempt == -1)
			continue;

		CONSTRAINT_DEBUG("trying constraint '%c'\n", constraint_attempt);

		if(constraint_attempt & MATCHING_CONSTRAINT_MASK_SHIFTED){
			/* matching constraint */
			const unsigned shift = MATCHING_CONSTRAINT_SHIFT;
			const unsigned mask = MATCHING_CONSTRAINT_MASK;
			unsigned match = ((cval->calculated_constraint >> shift) & mask) - 1;

			setupstate->error->operand = cval;

			if(entries[this_entry_i].is_output){
				setupstate->error->str = ustrprintf(
						"output operand has matching constraint");
				return;
			}

			if(match >= this_entry_i){
				setupstate->error->str = ustrprintf(
						"matching constraint (%u) out of bounds (must be < %ld)",
						match, (long)this_entry_i);
				return;
			}

			if(!entries[match].is_output){
				setupstate->error->str = ustrprintf(
						"matching constraint target is not an input");
				return;
			}

			cc->type = C_MATCH;
			cc->bits.match.cval = entries[match].cval;
			cc->bits.match.constraint = entries[match].cchosen;
			break; /* constraint met */

		}else{
			/* non-matching constraint - switch: */
		}

		switch(constraint_attempt){
				int chosen_reg;

			case CONSTRAINT_MASK_REG_a: chosen_reg = X86_64_REG_RAX; goto reg;
			case CONSTRAINT_MASK_REG_b: chosen_reg = X86_64_REG_RBX; goto reg;
			case CONSTRAINT_MASK_REG_c: chosen_reg = X86_64_REG_RCX; goto reg;
			case CONSTRAINT_MASK_REG_d: chosen_reg = X86_64_REG_RDX; goto reg;
			case CONSTRAINT_MASK_REG_D: chosen_reg = X86_64_REG_RDI; goto reg;
			case CONSTRAINT_MASK_REG_S: chosen_reg = X86_64_REG_RSI; goto reg;
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

			case CONSTRAINT_MASK_REG_any:
			case CONSTRAINT_MASK_REG_abcd:
			{
				/* 'q' / abcd register is actually any integer register on x64 */
				const int lim = (constraint_attempt == CONSTRAINT_MASK_REG_abcd
						? X86_64_REG_RDX + 1
						: MIN(regs->n, N_SCRATCH_REGS_I)); /* no floats */

				/* pick the first one - prioritised so this is fine */
				int found_reg = assign_constraint_pick_reg(
						cc, regs, regmask, lim, setupstate->fnty);

				if(!found_reg)
					continue; /* try again */
				break;
			}

			case CONSTRAINT_MASK_memory:
				cc->type = C_MEM;
				break;

			case CONSTRAINT_MASK_int_asm:
				/* link-time address or constant int */
				callback_gen_val(setupstate, cval);
				switch(cval->val->type){
					case V_LBL:
					case V_CONST_I:
						break; /* fall */
					default:
						continue; /* try again */
				}
				if(0){
			case CONSTRAINT_MASK_0_to_31:
			case CONSTRAINT_MASK_0_to_63:
			case CONSTRAINT_MASK_8bit_signed:
			case CONSTRAINT_MASK_0123:
			case CONSTRAINT_MASK_8bit_unsigned:
			case CONSTRAINT_MASK_int:
					callback_gen_val(setupstate, cval);
					if(!valid_int_constraint(constraint_attempt, cval->val))
						continue;
				}
				cc->type = C_CONST;
				break;

			case CONSTRAINT_MASK_REG_float:
				ICE("TODO: float");

			case CONSTRAINT_MASK_any:
				callback_gen_val(setupstate, cval);
				switch(cval->val->type){
					case V_CONST_F:
						ICE("TODO: float");

					case V_CONST_I:
						cc->type = C_CONST;
						break;

					case V_REG:
					{
						int found = 1;
						int i = cval->val->bits.regoff.reg.idx;

						/* any - attempt to use current register */
						if((regs->arr[i] & regmask) == 0){
							cc->type = C_REG;
							memcpy_safe(&cc->bits.reg, &cval->val->bits.regoff.reg);
						}else{
							const int lim = N_SCRATCH_REGS_I; /* no floats */
							found = assign_constraint_pick_reg(
									cc, regs, regmask, lim, setupstate->fnty);
						}

						if(found)
							break;
					} /* fall */

					case V_FLAG:
						cc->type = C_TO_REG_OR_MEM;
						break;

					case V_LBL:
					case V_REG_SPILT:
						cc->type = C_MEM;
						break;
				}
				break;
		}

		break; /* constraint met */
	}
}

static void assign_constraints(
		struct asm_setup_state *setupstate,
		struct constrained_pri_val *entries,
		size_t nentries)
{
	size_t i;
	for(i = 0; i < nentries; i++){
		/* pick the highest satisfiable constraint */
		assign_constraint(setupstate, entries, i);

		if(setupstate->error->str)
			break;
	}
}

static void calculate_constraints(
		struct asm_setup_state *setupstate,
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs,
		struct chosen_constraint *oconstraints,
		struct chosen_constraint *iconstraints)
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
	assign_constraints(setupstate, entries, nentries);

	free(entries);
}

static void constrain_input_matching(
		struct asm_setup_state *setupstate,
		struct chosen_constraint *constraint,
		struct constrained_val *cval)
{
	out_ctx *octx = setupstate->octx;

	switch(constraint->bits.match.constraint->type){
		case C_MATCH:
			ICE("match matching match");

		case C_MEM:
		{
			const out_val *v = constraint->bits.match.cval->val;

			assert(v && "should've generated memory operand for matching input");

			switch(v->type){
				case V_FLAG:
				case V_CONST_I:
				case V_CONST_F:
					ICE("can't constrain input val to %s", v_store_to_str(v->type));

				case V_LBL:
					out_val_retain(octx, v);
					out_val_retain(octx, cval->val);
					out_store(octx, v, cval->val);
					break;

				case V_REG:
				case V_REG_SPILT:
					out_val_retain(octx, v);
					cval->val = v_to_stack_mem(setupstate->octx, cval->val, v);
					break;
			}
			break;
		}

		case C_REG:
			cval->val = v_to_reg_given(
					octx, cval->val,
					&constraint->bits.match.constraint->bits.reg);
			break;

		case C_CONST:
		case C_TO_REG_OR_MEM:
			ICE("TODO");
			break;
	}
}

static void constrain_input_val(
		struct asm_setup_state *setupstate,
		struct chosen_constraint *constraint,
		struct constrained_val *cval)
{
	enum vto to_flags = TO_MEM;

	callback_gen_val(setupstate, cval);

	/* fill it with the right values */
	switch(constraint->type){
		case C_MATCH:
			constrain_input_matching(setupstate, constraint, cval);
			break;

		case C_TO_REG_OR_MEM:
			to_flags |= TO_REG;
			/* fall */
		case C_MEM:
			cval->val = v_to(setupstate->octx, cval->val, to_flags);
			break;

		case C_CONST:
			break;

		case C_REG:
			if(cval->val->type != V_REG
			|| cval->val->bits.regoff.reg.idx != constraint->bits.reg.idx)
			{
				/* don't freeup register */
				cval->val = v_to_reg_given(
						setupstate->octx, cval->val, &constraint->bits.reg);
			}
			break;
	}
}

static const out_val *temporary_for_output(
		struct asm_setup_state *setupstate,
		struct chosen_constraint *constraint,
		struct constrained_val *cval,
		type *ty)
{
	out_ctx *const octx = setupstate->octx;

	/* if the output already references the lvalue and matches the constraint,
	 * we can leave it.
	 *
	 * otherwise we construct an out_val that matches the constraint and use
	 * that as a temporary, then assign to the output afterwards
	 */
	switch(constraint->type){
		case C_MATCH:
			ICE("matching constraint for output?");

		case C_REG:
			/* need to use a temporary - a register can't match an lvalue
			 * e.g.
			 * __asm("mov $5, %0" : "=r"(...));
			 * can't use a register non-temporary here as we'd need to say
			 * mov $5, (%rax) which doesn't match the "r" constraint.
			 */
			return v_new_reg(octx, NULL, ty, &constraint->bits.reg);

		case C_TO_REG_OR_MEM:
		case C_MEM:
		{
			const out_val *spill;

			callback_gen_val(setupstate, cval);

			switch(cval->val->type){
				case V_FLAG:
				case V_CONST_I:
				case V_CONST_F:
					break;
				case V_REG:
				case V_LBL:
				case V_REG_SPILT:
					return NULL; /* matched */
			}

			spill = out_aalloc(octx, type_size(ty, NULL), type_align(ty, NULL), ty);

			return spill;
		}

		case C_CONST:
			assert(0 && "can't output to int/const");
	}

	assert(0);
}

static const out_val *initialise_output_temporary(
		out_ctx *octx,
		const out_val *out_temporary,
		struct chosen_constraint *constraint,
		const out_val *with_val)
{
	/* out_temporary is an uninitialised temporary,
	 * either memory or register. need to initialise it
	 * if "+" constraint (rw) present
	 *
	 * (and provided it's not the same memory as the lvalue)
	 */
	out_comment(octx, "read-write/\"+\" operand:");

	switch(constraint->type){
		case C_MATCH:
			ICE("matching constraint for output-temporary");

		case C_TO_REG_OR_MEM:
		case C_MEM:
			out_val_retain(octx, with_val);
			out_val_retain(octx, out_temporary);

			if(constraint->type == C_MEM)
				with_val = out_deref(octx, with_val);

			out_store(octx,
					/*dest*/out_deref(octx, out_temporary),
					/*src */with_val);
			break;

		case C_CONST:
			assert(0 && "output temporary const?");

		case C_REG:
		{
			struct vreg temporary_reg;

			assert(out_temporary->type == V_REG);

			/* need to get value in 'with_val'
				* into register 'out_temporary->bits.regoff.reg'
				*/
			memcpy_safe(&temporary_reg, &out_temporary->bits.regoff.reg);

			out_val_retain(octx, with_val);
			out_val_release(octx, out_temporary);

			out_temporary = v_to_reg_given(octx,
					out_deref(octx, with_val),
					&temporary_reg);
		}
	}

	return out_temporary;
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
				error->operand = NULL;
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
	for(i = 0; i < vals->n; i++){
		if(vals->arr[i].val){
			out_val_release(octx, vals->arr[i].val);
			vals->arr[i].val = NULL;
		}
	}
}

static void constrain_values(
		struct asm_setup_state *setupstate,
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs,
		struct chosen_constraint *coutputs,
		struct chosen_constraint *cinputs,
		const out_val *output_temporaries[])
{
	size_t const total = outputs->n + inputs->n;
	size_t i;

	for(i = 0; i < total; i++){
		struct chosen_constraint *constraint;

		if(i >= outputs->n){
			size_t input_i = i - outputs->n;

			assert(input_i < inputs->n);

			/* get this input into the memory/register/constant
			 * for the asm. if we can't, hard error */
			constraint = &cinputs[input_i];

			constrain_input_val(setupstate, constraint, &inputs->arr[input_i]);

			if(setupstate->error->str){
				setupstate->error->operand = &inputs->arr[input_i];
				return;
			}

		}else{
			const out_val *out_temporary;
			const int init_temporary
				= (outputs->arr[i].calculated_constraint & MODIFIER_MASK_rw);

			constraint = &coutputs[i];

			out_temporary = temporary_for_output(
					setupstate, constraint,
					&outputs->arr[i], outputs->arr[i].ty);
			/* may return null - in which case we reuse lvalue memory */

			if(out_temporary && init_temporary){
				callback_gen_val(setupstate, &outputs->arr[i]);

				out_temporary = initialise_output_temporary(
						setupstate->octx,
						out_temporary,
						constraint,
						outputs->arr[i].val);
			}

			output_temporaries[i] = out_temporary;
		}

		if(constraint->type == C_REG
		&& impl_reg_is_callee_save(setupstate->fnty, &constraint->bits.reg))
		{
			impl_use_callee_save(setupstate->octx, &constraint->bits.reg);
		}
	}
}

static void format_single_percent(
		char **const written_insn,
		const char **const p,
		size_t *const insn_len,
		struct constrained_val_array *outputs,
		const out_val *output_temporaries[],
		struct constrained_val_array *inputs,
		struct out_asm_error *error)
{
	size_t this_index;
	char *end;
	const out_val *oval;
	int deref = 0;

	if(**p == '['){
		ICE("TODO: named constraint");
	}

	this_index = strtol(*p, &end, 0);
	if(end == *p){
		error->str = ustrprintf(
				"invalid register character '%c', number expected",
				**p);
		return;
	}
	*p = end - 1; /* ready for ++ */

	if(this_index >= outputs->n){
		this_index -= outputs->n;

		if(this_index > inputs->n){
			error->str = ustrprintf(
					"invalid register index %d",
					(int)this_index);
			return;
		}

		oval = inputs->arr[this_index].val;
	}else{
		if((oval = output_temporaries[this_index])){
			/* fine */
		}else{
			oval = outputs->arr[this_index].val;
			deref = 1;
		}
	}


	if(!deref)
		deref = (oval->type == V_REG_SPILT);

	{
		const char *val_str;
		char *op;
		size_t oplen;

		val_str = impl_val_str(oval, deref);
		oplen = strlen(val_str);

		op = dynvec_add_n(written_insn, insn_len, oplen);

		memcpy(op, val_str, oplen);
	}
}

static char *format_insn(const char *format,
		struct constrained_val_array *outputs,
		const out_val *output_temporaries[],
		struct constrained_val_array *inputs,
		struct out_asm_error *error)
{
	char *written_insn = NULL;
	size_t insn_len = 0;
	const char *p;

	for(p = format; *p; p++){
		if(*p == '%'){
			switch(*++p){
				case '%':
					*(char *)dynvec_add(&written_insn, &insn_len) = '%';
					break;

				case '=':
				{
					unsigned v = out_label_uniq_asm();
					int len;
					char buf[32];
					char *dest;

					len = snprintf(buf, sizeof buf, "%u", v);
					assert(len > 0);

					dest = dynvec_add_n(&written_insn, &insn_len, len);
					memcpy(dest, buf, len);
					break;
				}

				default:
					format_single_percent(
							&written_insn,
							&p,
							&insn_len,
							outputs,
							output_temporaries,
							inputs,
							error);
			}
			if(error->str)
				break;
		}else{
			*(char *)dynvec_add(&written_insn, &insn_len) = *p;
		}
	}

	*(char *)dynvec_add(&written_insn, &insn_len) = '\0';

	return written_insn;
}

static void init_used_regs(out_ctx *octx, struct regarray *regs)
{
	out_val_list *it;
	for(it = octx->val_head; it; it = it->next){
		const out_val *this = &it->val;
		if(this->retains && this->type == V_REG){
			const struct vreg *vr = &this->bits.regoff.reg;
			if(!vr->is_float)
				regs->arr[vr->idx] |= REG_USED_IN | REG_USED_OUT;
		}
	}
}

static void debug_used_regs(out_ctx *octx, struct regarray *regs)
{
	int i;
	for(i = 0; i < regs->n; i++){
		if(regs->arr[i]){
			struct vreg reg;
			const char *desc = "out";

			if(regs->arr[i] & (REG_USED_IN | REG_USED_OUT))
				desc = "in | out";
			else if(regs->arr[i] & REG_USED_IN)
				desc = "in";

			reg.idx = i;
			reg.is_float = 0;

			out_comment(octx, "regs[%s] = %s", impl_reg_str(&reg), desc);
		}
	}
}

void out_inline_asm_ext_begin(
		out_ctx *octx,
		struct inline_asm_parameters *asm_params,
		struct inline_asm_state *st,
		struct out_asm_error *error)
{
#define format asm_params->format
#define clobbers asm_params->clobbers
#define loc asm_params->where
	size_t i;
	struct regarray regs;
	char *escaped_fname;
	char *insn = NULL;
	struct asm_setup_state setupstate;

	st->constraints.inputs = umalloc(asm_params->inputs->n * sizeof *st->constraints.inputs);
	st->constraints.outputs = umalloc(asm_params->outputs->n * sizeof *st->constraints.outputs);

	st->output_temporaries = umalloc(asm_params->outputs->n * sizeof *st->output_temporaries);

	regs.arr = v_alloc_reg_reserve(octx, &regs.n);

	init_used_regs(octx, &regs);

	setupstate.octx = octx;
	setupstate.gen_callback = asm_params->gen_callback;
	setupstate.gen_callback_ctx = asm_params->gen_callback_ctx;
	setupstate.regs = &regs;
	setupstate.fnty = asm_params->fnty;
	setupstate.error = error;

	/* first, spill all the clobber registers,
	 * then we can have a valid list of registers active at asm() time
	 */
	parse_clobbers(clobbers, &regs, error);
	if(error->str) goto error;

	calculate_constraints(
			&setupstate,
			asm_params->outputs, asm_params->inputs,
			st->constraints.outputs, st->constraints.inputs);
	if(error->str) goto error;

	if(fopt_mode & FOPT_VERBOSE_ASM)
		debug_used_regs(octx, &regs);

	constrain_values(
			&setupstate,
			asm_params->outputs,
			asm_params->inputs,
			st->constraints.outputs,
			st->constraints.inputs,
			st->output_temporaries);
	if(error->str) goto error;

	insn = format_insn(
			format,
			asm_params->outputs,
			st->output_temporaries,
			asm_params->inputs,
			error);

	if(error->str) goto error;

	out_comment(octx, "### actual inline");

	/* location information */
	escaped_fname = str_add_escape(loc->fname, strlen(loc->fname));
	out_asm2(octx, SECTION_TEXT, P_NO_INDENT, "# %d \"%s\"", loc->line, escaped_fname);
	free(escaped_fname), escaped_fname = NULL;

	out_asm(octx, "%s", insn ? insn : "");
	out_asm2(octx, SECTION_TEXT, P_NO_INDENT, "# 0 \"\"");

	/* consume inputs */
	out_asm_release_valarray(octx, asm_params->inputs);

	if(0){
error:
		/* release temporaries */
		for(i = 0; i < asm_params->outputs->n; i++){
			if(st->output_temporaries[i]){
				out_val_release(octx, st->output_temporaries[i]);
				st->output_temporaries[i] = NULL;
			}
		}
	}

	free(insn), insn = NULL;

	free(regs.arr);
#undef format
#undef clobbers
#undef loc
}

void out_inline_asm_ext_output(
		out_ctx *octx,
		const size_t i,
		struct constrained_val *output,
		struct inline_asm_state *st)
{
	const out_val *val = output->val;

	if(st->output_temporaries[i]){
		constrain_output(octx,
				val,
				st->output_temporaries[i],
				&st->constraints.outputs[i]);

	}else{
		out_val_release(octx, val);
	}

	output->val = NULL;
}

void out_inline_asm_ext_end(struct inline_asm_state *st)
{
	free(st->output_temporaries);
	free(st->constraints.inputs);
	free(st->constraints.outputs);
}

void out_inline_asm(out_ctx *octx, const char *insn)
{
	out_asm(octx, "%s", insn);
}
