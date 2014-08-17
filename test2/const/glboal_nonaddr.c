// RUN: %ocheck 0 %s

int a, b;

f()
{
	a = a || b; // ensure this isn't confused for &a || &b
}

g()
{
	a = &a || &b;
}

main()
{
	f();
	if(a || b)
		abort();

	g();
	if(!a || b)
		abort();

	return 0;
}
