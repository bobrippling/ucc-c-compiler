typedef struct A
{
	long i, j;
	char c;
} A;

A f()
{
	return (A){1,2};
}

main()
{
	A a = g() ? f() : (A){3,4};
	h(&a);
}
