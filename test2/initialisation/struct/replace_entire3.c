struct A
{
	struct B
	{
		int i, j;
	} b;
	int k;
};

int f()
{
	struct B b = { 1, 2 };
	struct A a = { .b = b, .b.i = 2 }; // .b = b is dropped
	return a.b.j; // 0
}

main()
{
	return f();
}
