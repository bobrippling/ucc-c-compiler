// RUN: %ucc -c %s
main()
{
	int f();
	int x[2];

	q(&f);
	q(&x);
}
