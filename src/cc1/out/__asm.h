#ifndef __ASM_H
#define __ASM_H

/* __asm__ code */
void out_constraint_check(where *w, const char *constraint, int output);
void out_constrain(asm_inout *io);
void out_asm_inline(asm_args *cmd);

#endif
