g()
{
	printf("g()\n");
	return 3;
}

int (*f(int a[g()]))[] // called once
{
	int (*p)[g()] = a; // called once

	return p + 1;
}

main()
{
	int a[2][g()];

	a[0][0] = 0;
	a[0][1] = 1;
	a[0][2] = 2;

	a[1][0] = 3;
	a[1][1] = 4;
	a[1][2] = 5;

	int (*p)[] = f(a);

	return (*p)[1]; // 4
}
