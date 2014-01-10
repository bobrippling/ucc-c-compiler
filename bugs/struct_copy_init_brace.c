struct A
{
	struct B
	{
		int i, j;
	} b, c;
};

f(struct B *b)
{
	struct A fine = {
		.b = *b // XXX: should be no missing braces warning
	};

	struct A bad = {
		.b = { *b }
	};

}
