// RUN: %ocheck 6 %s

main()
{
	struct A
	{
		int i, j, k;
	} a = { 1, 2, 3 }, b = a;

	return b.i + b.j + b.k;
}
