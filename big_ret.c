struct A
{
	long a, b, c;
};

struct A f()
#ifdef IMPL
{
	return (struct A){ 1, 2, 3 };
}
#else
;

main()
{
	struct A a = f();

	return a.a + a.b + a.c;
}
#endif
