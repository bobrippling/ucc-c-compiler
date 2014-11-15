// RUN: %archgen %s 'x86_64:movq (%%rax), %%rcx'

struct A
{
	long a, b, c, d;
};

f(volatile struct A *p)
{
	*p;
}

main()
{
	f(0);
}
