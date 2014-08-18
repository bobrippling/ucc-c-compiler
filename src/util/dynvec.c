#include "dynvec.h"
#include "alloc.h"

#undef dynvec_add
#undef dynvec_add_n

void *dynvec_add_n(void *any_ptr, size_t *count, size_t sz1, size_t n)
{
	const size_t orig_sz = *count * sz1;

	void **typed_ptr = any_ptr;
	size_t size = (*count += n) * sz1;

	*typed_ptr = urealloc1(*typed_ptr, size);

	return (char *)*typed_ptr + orig_sz;
}

void *dynvec_add(void *any_ptr, size_t *count, size_t sz1)
{
	return dynvec_add_n(any_ptr, count, sz1, 1);
}
