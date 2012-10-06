extern int x[][2][4];

main()
{
	return x[1][2][3]; // only one dereference
}
