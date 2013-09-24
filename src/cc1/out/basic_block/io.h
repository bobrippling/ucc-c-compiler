#ifndef BASIC_BLOCK_INT_H
#define BASIC_BLOCK_INT_H

void bb_add(basic_blk *, const char *, ...);
void bb_addv(basic_blk *, const char *, va_list);
void bb_commentv(basic_blk *, const char *, va_list);

#define bb_cmd(f, s, ...) fprintf(f, "\t" s "\n", __VA_ARGS__)

#endif
