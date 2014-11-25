//#include <sys/types.h>
typedef unsigned long size_t;

void *memcpy(void *dst, const void *src, size_t size)
{
	void *orig = dst;
	__asm volatile("rep ; movsq"
			:"=D" (dst), "=S" (src)
			:"0" (dst), "1" (src), "c" (size >> 3)
			:"memory");
	__asm volatile("rep ; movsb"
			:"=D" (dst), "=S" (src)
			:"0" (dst), "1" (src), "c" (size & 7)
			:"memory");
	return orig;
}
