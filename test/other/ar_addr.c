// RUN: %ocheck 2 %s

main()
{
	int ar[10];
	int *p;

	p = &ar[5];

	*p = 2;

	return ar[5];
}
