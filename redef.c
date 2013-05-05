typedef unsigned long size_t;

f()
{
	typedef short size_t;
	size_t x;

	_Static_assert(sizeof(x) == 2, "short typedef missed");
}

g()
{
	size_t a;
	typedef int a; // CHECK: /error: redef/
}

h()
{
	typedef int t;
	{
		int t;
		t = 2;
		return t;
	}
}
