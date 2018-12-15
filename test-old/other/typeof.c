main()
{
	int *x = (__typeof(x))2;
	__typeof(*x) i;

	i = (__typeof(int))x;

	assert(i == 2);
}
