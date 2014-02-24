// RUN: %ocheck 7 %s

struct A
{
	int i, j;
};

f(struct A *p)
{
	struct A a = *p;

	return a.i + a.j;
}

main()
{
	struct A x;
	x.i = 5;
	x.j = 2;
	return f(&x);
}
