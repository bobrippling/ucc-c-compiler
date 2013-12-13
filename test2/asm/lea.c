// RUN: echo TODO: false
void test(int a, uint8_t *pixels)
{
	__asm volatile(
			"lea (%1, %0), %%eax	\n\t"
			: "+r" (pixels)
			: "r" (a+64)
			: "memory"
			);
}
