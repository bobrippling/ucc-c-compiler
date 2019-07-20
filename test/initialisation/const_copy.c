// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

typedef struct a
{
	int i, j;
} A;

void check(A *p)
{
	if(p->i != 1)
		abort();
	if(p->j)
		abort();
}

main()
{
	A a = { 1 };
	const A b = a; // init const
	A c = b; // init from const

	check(&a);
	check(&b);
	check(&c);

	return 0;
}
