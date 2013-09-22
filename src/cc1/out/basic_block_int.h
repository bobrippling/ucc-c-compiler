#ifndef BASIC_BLOCK_INT_H
#define BASIC_BLOCK_INT_H

void bb_add(basic_blk *, const char *, ...);
void bb_addv(basic_blk *, const char *, va_list);

#define bb_label(b, lbl) bb_add(b, "%s:", lbl)

#endif
