#ifndef OUT__ASM_H
#define OUT__ASM_H

/* query the backend - is this constraint valid? */
void out_asm_constraint_check(
		where *w, const char *constraint, int is_output);

struct constrained_val
{
	const out_val *val;
	const char *constraint;
};

struct out_asm_error
{
	char *str;
	struct constrained_val *operand;
};

/* output the constraint cmd, with %0 replaced, etc */
void out_inline_asm_extended(
		out_ctx *, const char *insn,
		struct constrained_val *outputs, const size_t noutputs,
		struct constrained_val *inputs, const size_t ninputs,
		char **clobbers, struct out_asm_error *error);

void out_inline_asm(out_ctx *, const char *insn);

#endif
