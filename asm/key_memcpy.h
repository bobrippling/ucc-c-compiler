typedef unsigned char uint8_t;

mov(uint8_t *dst, const uint8_t *src)
{
	asm volatile (
			"movdqu    (%[src]), %%xmm0\n\t"
			"movdqu  16(%[src]), %%xmm1\n\t"
			"movdqu  32(%[src]), %%xmm2\n\t"
			"movdqu  48(%[src]), %%xmm3\n\t"
			"movdqu  64(%[src]), %%xmm4\n\t"
			"movdqu  80(%[src]), %%xmm5\n\t"
			"movdqu  96(%[src]), %%xmm6\n\t"
			"movdqu 112(%[src]), %%xmm7\n\t"
			"movdqu %%xmm0,    (%[dst])\n\t"
			"movdqu %%xmm1,  16(%[dst])\n\t"
			"movdqu %%xmm2,  32(%[dst])\n\t"
			"movdqu %%xmm3,  48(%[dst])\n\t"
			"movdqu %%xmm4,  64(%[dst])\n\t"
			"movdqu %%xmm5,  80(%[dst])\n\t"
			"movdqu %%xmm6,  96(%[dst])\n\t"
			"movdqu %%xmm7, 112(%[dst])"
			:
			: [src] "r" (src),
			  [dst] "r" (dst)
			: "xmm0", "xmm1", "xmm2", "xmm3",
			  "xmm4", "xmm5", "xmm6", "xmm7",
			  "memory");
}
