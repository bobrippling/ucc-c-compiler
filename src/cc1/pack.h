#ifndef PACK_H
#define PACK_H

int  pack_to_align(int o, int align);
int  pack_to_word(int o);
void pack_next(int *poffset, int *after_space, int sz, int align);

#endif
