#ifndef DYNVEC_H
#define DYNVEC_H

#include <stddef.h>

void *dynvec_sz_add(void *p, size_t *, size_t sz1);

#define dynvec_add(p, szp) \
	dynvec_sz_add((p), (szp), sizeof(**(p)))

#endif
