// RUN: %ocheck 0 %s

static int expected;

void abort();

static void test(int v)
{
	if(v != expected)
		abort();
}

void noop(int a)
{
}

static void f(char a)
{
	noop(0x12345678); // setup arg0 register with some dummy value
	test(!!a);
}

static void g(char a)
{
	noop(0x12345678);
	test(+(a != 0));
}

int main()
{
	expected = 1;
	f(2);

	expected = 0;
	f(0);

	expected = 1;
	g(3);

	expected = 0;
	g(0);
}
