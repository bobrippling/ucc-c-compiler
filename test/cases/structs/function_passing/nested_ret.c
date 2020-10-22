// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

struct A
{
	struct B
	{
		float a;
	} b;
	double x;
};

struct A f(void)
{
	return (struct A){ 1, 2 };
}

int main()
{
	// this currently works, but doesn't match the platform ABI
	struct A a = f();

	if(a.b.a != 1)
		abort();
	if(a.x != 2)
		abort();

	return 0;
}
