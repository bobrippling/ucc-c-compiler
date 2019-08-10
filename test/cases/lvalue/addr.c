// RUN: %ucc -c %s

void q(void *);

main()
{
	int f();
	int x[2];

	q(&f);
	q(&x);
}
