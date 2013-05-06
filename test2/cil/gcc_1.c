// RUN: echo TODO; false
main()
{
	int x, y, z;
	return &(x ? y : z) - & (x++, x);
}
