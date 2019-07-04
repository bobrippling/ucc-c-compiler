// RUN: %ocheck 0 %s

__attribute((always_inline))
inline int get_ebx()
{
	int ebx;
	__asm("movl %%ebx, %0" : "=mr"(ebx));
	return ebx;
}

__attribute((always_inline))
inline void set_ebx(int new)
{
	__asm("movl %0, %%ebx" : : "rmi"(new));
}

f()
{
	__asm volatile("" ::: "ebx");
}

g()
{
	int i;
	__asm("" : "=b"(i));
	return i;
}

h()
{
	__asm volatile("" : : "b"(0));
}

main()
{
	set_ebx(5);

	if(get_ebx() != 5)
		abort();

	f();
	if(get_ebx() != 5)
		abort();
	g();
	if(get_ebx() != 5)
		abort();
	h();
	if(get_ebx() != 5)
		abort();

	return 0;
}
