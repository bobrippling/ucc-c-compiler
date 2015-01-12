f(volatile int i)
{
	i = 3;
	i = g();
	i += 2;
	return i;
}
