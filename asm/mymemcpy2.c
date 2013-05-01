void *memcpy(void *d, void *s, unsigned long n)
{
	void *const r = d;

	asm("cld\n"
			"\trepnz movsb\n"
			: /* no output */
			: "D" (d), "S" (s), "c" (n)
			: "rdi", "rsi", "memory" /* clobber */
			);

	return r;
}
