#ifndef PACK_H
#define PACK_H

int  pack_to_align(int o, int align);
int  pack_to_word(int o);
void pack_next(
		unsigned *poffset, unsigned *after_space,
		unsigned sz, unsigned align);

#endif
