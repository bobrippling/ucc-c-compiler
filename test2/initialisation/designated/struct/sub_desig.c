// RUN: %ocheck 5 %s

struct A
{
	struct B
	{
		int i, j;
	} b;
} a = {
	.b.j = 5
};

main()
{
	return a.b.i + a.b.j;
}
