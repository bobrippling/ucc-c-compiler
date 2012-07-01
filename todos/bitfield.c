main()
{
	struct A
	{
		int i : 4, j : 4;
		int a, b : 2, : 2;
	} a;

	a.i = 5; /* assigns to a.{i,j} generating bitmask instructions */
}
