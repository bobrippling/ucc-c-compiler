#ifndef PACK_H
#define PACK_H

unsigned long pack_to_align(unsigned long, unsigned align);
unsigned long pack_to_word(unsigned long);

void pack_next(
		unsigned long *poffset,
		unsigned long *after_space,
		unsigned sz, unsigned align);

#endif
