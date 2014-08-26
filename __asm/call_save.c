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
	f();
	g();
	h();
}
