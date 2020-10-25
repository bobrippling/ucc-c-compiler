// RUN: %ocheck 0 %s

int called;

int x()
{
	called = 1;
}

int (*f(void))()
{
	return x;
}

main()
{
	f()();
	return called ? 0 : 1;
}
