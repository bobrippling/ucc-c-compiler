// RUN: %archgen %s 'x86_64:movq (%%rax), %%rcx'
// RUN: %archgen %s 'x86_64:movq (%%rax), %%rcx' -DTO_VOID

struct A
{
	long a, b, c, d;
};

f(volatile struct A *p)
{
#ifdef TO_VOID
	(void)
#endif
	*p;
}

main()
{
	f(0);
}
