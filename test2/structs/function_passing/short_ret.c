// RUN: %ocheck 6 %s

struct A
{
	int a, b, c;
};

struct A f(void)
{
	return (struct A){ 1, 2, 3 };
}

main()
{
	struct A a = f();
	return a.a + a.b + a.c;
}
