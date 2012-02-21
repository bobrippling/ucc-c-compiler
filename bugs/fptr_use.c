for_all(int *a, void (*f)(int))
{
	int *i;
	for(i = a; *i; i++)
		f(*i);
}

void pi(int x)
{
	printf("%d\n", x);
}

main()
{
	int tim[10];
	int i;

	for(i = 0; i < 9; i++)
		tim[i] = i + 1;
	tim[i] = 0;

	for_all(tim, pi);
}
