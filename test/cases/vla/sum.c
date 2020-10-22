// RUN: %ocheck 15 %s

sum(long *p, int bytes, int tysz)
{
	int t = 0;
	for(int i = 0; i < bytes/tysz; i++)
		t += p[i];
	return t;
}

init(long *p, int n)
{
	for(int i = 0; i < n; i++)
		p[i] = i;
}

f(int n)
{
	long ar[3 * n]; // 8 * 2 * 3 = 48

	init(ar, sizeof ar / sizeof *ar);

	return sum(ar, sizeof ar, sizeof(long));
}

main()
{
	return f(2);
}
