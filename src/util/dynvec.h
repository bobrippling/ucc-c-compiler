#ifndef DYNVEC_H
#define DYNVEC_H

#include <stddef.h>

void *dynvec_add(void *p, size_t *out_count, size_t sz1);
void *dynvec_add_n(void *p, size_t *out_count, size_t sz1, size_t n);

#define dynvec_add(p, countp) \
	dynvec_add((p), (countp), sizeof(**(p)))

#define dynvec_add_n(p, countp, n) \
	dynvec_add_n((p), (countp), sizeof(**(p)), (n))

#endif
