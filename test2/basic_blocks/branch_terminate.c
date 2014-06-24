// RUN: %jmpcheck %s

main()
{
	int x;

	x = 3;

	if(x)
		return 3;

	x = 4;
}
