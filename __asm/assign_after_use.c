void f(unsigned in, unsigned *out)
{
	// asm writes %eax - we must infer that it's clobbered
	// i.e. not generate *out until after the inline asm
	__asm(
			"mov %1, %%eax\n\t"
			"mov %%eax, %0"
			: "=a"(*out)
			: "ir"(in));
}

main()
{
	unsigned i;
	f(0, &i);
	return i;
}
