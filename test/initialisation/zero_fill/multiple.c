// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

g()
{
	struct { int a, b; } a = { 2 }, b = {};
	if(a.b || b.a || b.b)
		abort();
}

h()
{
	int y[3] = { }; // extension, also for structs

	if(y[0] || y[1] || y[2])
		abort();
}

main()
{
	g();
	h();
	return 0;
}
