f(int n)
{
	static int x[n]; // invalid
	static int (*p)[n]; // valid
}
