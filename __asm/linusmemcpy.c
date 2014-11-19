#include <sys/types.h>

void *memcpy(void *dst, const void *src, size_t size)
{
	void *orig = dst;
	asm volatile("rep ; movsq"
			:"=D" (dst), "=S" (src)
			:"0" (dst), "1" (src), "c" (size >> 3)
			:"memory");
	asm volatile("rep ; movsb"
			:"=D" (dst), "=S" (src)
			:"0" (dst), "1" (src), "c" (size & 7)
			:"memory");
	return orig;
}
