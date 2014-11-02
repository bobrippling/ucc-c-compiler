typedef _Bool BOOL;

enum { FALSE, TRUE };

typedef unsigned uint32_t;

BOOL cpuid(uint32_t *_eax, uint32_t *_ebx, uint32_t *_ecx, uint32_t *_edx)
{
	static BOOL cpuid_support = FALSE;
	if (!cpuid_support) {
		uint32_t pre_change, post_change;
		const uint32_t id_flag = 0x200000;
		__asm ("pushfl\n\t" /* Save %eflags to restore later. */
				"pushfl\n\t" /* Push second copy, for manipulation. */
				"popl %1\n\t" /* Pop it into post_change. */
				"movl %1,%0\n\t" /* Save copy in pre_change. */
				"xorl %2,%1\n\t" /* Tweak bit in post_change. */
				"pushl %1\n\t" /* Push tweaked copy... */
				"popfl\n\t" /* ... and pop it into %eflags. */
				"pushfl\n\t" /* Did it change? Push new %eflags... */
				"popl %1\n\t" /* ... and pop it into post_change. */
				"popfl" /* Restore original value. */
				: "=&r" (pre_change), "=&r" (post_change)
				: "ir" (id_flag));
		if (((pre_change ^ post_change) & id_flag) == 0)
			return FALSE;
		else
			cpuid_support = TRUE;
	}
	__asm volatile(
			"cpuid"
			: "=a" (*_eax),
			"=b" (*_ebx),
			"=c" (*_ecx),
			"=d" (*_edx)
			: "0" (*_eax), "2" (*_ecx));
	return TRUE;
}
