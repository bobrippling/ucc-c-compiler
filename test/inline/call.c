// RUN: %ocheck 3 %s -finline-functions -fno-semantic-interposition
// RUN: %check %s -fshow-inlined -finline-functions -fno-semantic-interposition

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
