struct { int x, y; } x[] = {
	[2].x = 1,
	[2]   = 2,
};
/*
main()
{
	printf("%d\n", x[2].x);
}
*/
