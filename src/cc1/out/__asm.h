#ifndef OUT__ASM_H
#define OUT__ASM_H

struct constrained_val
{
	const out_val *val;
	type *ty;
	unsigned calculated_constraint;
};

struct constrained_val_array
{
	struct constrained_val *arr;
	size_t n;
};

void out_asm_release_valarray(
		out_ctx *octx, struct constrained_val_array *vals);

struct out_asm_error
{
	char *str;
	struct constrained_val *operand;
};

/* constraint init */
unsigned out_asm_calculate_constraint(
		const char *constraint,
		const int is_output,
		struct out_asm_error *error);

struct inline_asm_state
{
	struct
	{
		struct chosen_constraint *inputs, *outputs;
	} constraints;
	const out_val **output_temporaries;
};

/* output the constraint cmd, with %0 replaced, etc */
void out_inline_asm_ext_begin(
		out_ctx *, const char *insn,
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs,
		char **clobbers,
		const where *,
		type *fnty,
		struct out_asm_error *error,
		struct inline_asm_state *state);

void out_inline_asm_ext_output(
		out_ctx *octx,
		const size_t output_i,
		struct constrained_val *output,
		struct inline_asm_state *);

void out_inline_asm_ext_end(struct inline_asm_state *);

void out_inline_asm(out_ctx *, const char *insn);

#endif
