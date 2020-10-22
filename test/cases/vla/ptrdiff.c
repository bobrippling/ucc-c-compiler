// RUN: %ocheck 1 %s

f()
{
	int n = 4, m = 3;
	int a[n][m];
	int (*p)[m] = a;  // p == &a[0]
	p += 1; // p = (char *)a + 12
	(*p)[2] = 99;
	n = p - a; // p == &a[1] // a[1][2] == 99
	// n == 1

	return n;
}

main()
{
	return f();
}
