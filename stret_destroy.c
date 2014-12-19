typedef struct { int i; } A;

clean(A *p)
{
	p->i = 999;
}

A f()
{
	A a __attribute((cleanup(clean))) = { 3 };

	return a;
}

main()
{
	A a = f();

	printf("should be 3: %d\n", a.i);
}
