// RUN: %check %s -fshow-inlined -finline-functions -fno-semantic-interposition
// RUN: %ocheck 20 %s -finline-functions -fno-semantic-interposition

f(int i)
{
	return 3 + i;
}

g(int i)
{
	return 2 * f(i); // CHECK: note: function inlined
}

main()
{
	return g(7); // CHECK: note: function inlined
}
