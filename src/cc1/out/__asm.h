#ifndef OUT__ASM_H
#define OUT__ASM_H

struct constrained_val
{
	const out_val *val;
	unsigned calculated_constraint;
};

struct constrained_val_array
{
	struct constrained_val *arr;
	size_t n;
};

struct out_asm_error
{
	char *str;
	struct constrained_val *operand;
	char *warning;
};

/* constraint init */
void out_asm_calculate_constraint(
		struct constrained_val *cval,
		const char *constraint,
		const int is_output,
		struct out_asm_error *error);

/* output the constraint cmd, with %0 replaced, etc */
void out_inline_asm_extended(
		out_ctx *, const char *insn,
		struct constrained_val_array *outputs,
		struct constrained_val_array *inputs,
		char **clobbers, struct out_asm_error *error);

void out_inline_asm(out_ctx *, const char *insn);

#endif
