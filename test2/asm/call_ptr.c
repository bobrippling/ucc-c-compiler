// RUN: %ucc -c %s

main()
{
	extern int (*f)(int, int);
	extern int   q( int, int);
	int (*p)(int, int);

	f(1, 2);
	(*f)(1, 2);
	(*****f)(1, 2);

	q(1, 2);

	p = q;
	p(1, 2);

	p = f;
	p(1, 2);
}
