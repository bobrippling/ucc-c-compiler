#ifndef __ASM_H
#define __ASM_H

/* __asm__ code */

/* check the constraint string */
void out_constraint_check(where *w, const char *constraint, int output);

/* output the constraint cmd, with %0 replaced, etc
 * this also pops all args
 */
void out_asm_inline(asm_args *cmd, where *const err_w);

/* move vtop into the appropriate input constrain area */
void out_constrain_top(asm_inout *io);

#endif
