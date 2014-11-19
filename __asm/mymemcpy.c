#include <sys/types.h>

void *memcpy(void *dst, const void *src, size_t n)
{
	void *const orig = dst;

	asm("rep movsb"
			: "D" (dst), "S" (src)
			: "D" (dst), "S" (src), "c" (n)
			: "memory"
		);

	return orig;
}
