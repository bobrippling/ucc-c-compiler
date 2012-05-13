main()
{
	struct A
	{
		int i : 4, j : 4;
	} a;

	a.i = 5; /* assigns to a.{i,j} generating bitmask instructions */
}
