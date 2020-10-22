// RUN: %ocheck 12 %s

int g(void)
{
	return 9;
}

void f(int *p)
{
	// need to spill *p
  *p += g();
}

main()
{
	int i = 3;
	f(&i);
	return i;
}
