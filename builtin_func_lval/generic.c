f(int k)
{
	int i;
	int *p = &_Generic(0, int: i);

	*p = 2;

	return i;
}
