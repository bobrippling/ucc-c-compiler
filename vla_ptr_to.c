f()
{
	int n = 4, m = 3;
	int a[n][m];
	int (*p)[m] = a;  // p == &a[0]
	p += 1;
	(*p)[2] = 99;
	n = p - a; // p == &a[1] // a[1][2] == 99 //n==1

	return n;
}

/* bad: */
g()
{
	int n = 5, m = 6;
	int (*a)[n];
	int (*b)[m];

	return a - b; // different types???
}

main()
{
	return f();
}
