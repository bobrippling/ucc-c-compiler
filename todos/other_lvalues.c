main()
{
	int i;

	_Generic(2, int: i, char: *(int *)5) = 2; // C99 lvalue
	({ i; }) = 2; // GNU lvalue

	return i;
}
