// RUN: %ucc %s

main()
{
	int *p, *q;

	*(p + 1);

	*p + 1;

	p - q;

	p < q ? 1 : 0;
}
