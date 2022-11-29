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

void clobber_ebx()
{
	__asm volatile("" ::: "ebx");
}

int clobber_ebx2()
{
	int i;
	__asm("" : "=b"(i));
	return i;
}

void h()
{
	__asm volatile("" : : "b"(0));
}

void check1()
{
	set_ebx(5);

	if(get_ebx() != 5)
		abort();

	clobber_ebx();
	if(get_ebx() != 5)
		abort();
	clobber_ebx2();
	if(get_ebx() != 5)
		abort();
	h();
	if(get_ebx() != 5)
		abort();
}

int use_ebx()
{
	int ebx_temp;
	// having "m" here causes the bug, as the asm() temp spill conflicts
	__asm("mov %1, %0" : "=b"(ebx_temp) : "m"(3));
	return ebx_temp;
}

void check2()
{
	__asm("mov %0, %%ebx" : : "g"(72));

	if(use_ebx() != 3)
		abort();

	int ebx_after;
	__asm("mov %%ebx, %0" : "=g"(ebx_after));

	if(ebx_after != 72)
		abort();
}

main()
{
	check1();
	check2();
	return 0;
}
