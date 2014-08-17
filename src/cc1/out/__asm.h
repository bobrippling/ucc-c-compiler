#ifndef OUT__ASM_H
#define OUT__ASM_H

/* query the backend - is this constraint valid? */
void out_asm_constraint_check(
		where *w, const char *constraint, int is_output);


/* output the constraint cmd, with %0 replaced, etc */
void out_asm_inline(
		out_ctx *, const char *insn,
		const out_val **inputs,
		const out_val **outputs,
		char **clobbers);

#endif
