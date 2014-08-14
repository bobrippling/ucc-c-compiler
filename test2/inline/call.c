// RUN: %ocheck 3 %s
// RUN: %check %s -fshow-inlined

inc(int x)
{
	return x + 1;
}

call(int (*p)(int), int arg)
{
	return p(arg);
}

main()
{
	return call(inc, 2); // CHECK: note: function inlined
}
