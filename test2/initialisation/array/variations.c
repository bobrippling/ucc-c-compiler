// RUN: %ocheck 0 %s

chk(int (*p)[2], int a, int b, int c, int d)
{
	if(p[0][0] != a
	|| p[0][1] != b
	|| p[1][0] != c
	|| p[1][1] != d)
		abort();
}

main()
{
	int a[][2] = { 0, 1, 2, 3 };
	int b[][2] = { {1}, {2} };
	int c[][2] = { {1}, 2, 3 };
	int d[][2] = { 1, 2, {3} };

	chk(a, 0, 1, 2, 3);
	chk(b, 1, 0, 2, 0);
	chk(c, 1, 0, 2, 3);
	chk(d, 1, 2, 3, 0);

	return 0;
}
