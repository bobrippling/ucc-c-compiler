// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

struct A
{
	int i, j, k;
};

f(struct A *p)
{
	*p = (struct A){ 1, .k = 3 };
}

main()
{
	struct A a;
	f(&a);

	if(a.i != 1 || a.j != 0 || a.k != 3)
		abort();

	return 0;
}
