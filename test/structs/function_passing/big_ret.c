// RUN: %ocheck 6 %s

struct A
{
	long a, b, c;
};

struct A f()
{
	return (struct A){ 1, 2, 3 };
}

main()
{
	struct A a = f();

	return a.a + a.b + a.c;
}
