// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

called;

g()
{
	called = 1;
	return 3;
}

void f(int vla[2][g()])
{
}

main()
{
	f(0);

	if(!called)
		abort();

	return 0;
}
