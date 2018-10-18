#ifndef IMPL_JMP_H
#define IMPL_JMP_H

#include "asm.h"

/* this is the implementation backend which handles flow control. the front
 * part of the backend (out.c and co) don't use this, it's just used by the
 * blk.c flow logic part */

void impl_jmp(enum section_type, const char *lbl);

#endif
