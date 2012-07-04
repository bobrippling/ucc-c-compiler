main()
{
	struct A
	{
		int i : 4, j : 4;
		int a, b : 2, : 2;
	} a;

	struct
	{
		int a : 1;
		int b : 1;
	} x;


	a.i = 5; /* assigns to a.{i,j} generating bitmask instructions */

	x.b = 5; /* overflow */

	return x.b;
}
