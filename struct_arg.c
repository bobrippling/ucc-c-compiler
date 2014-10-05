struct A
{
	int i, j;
	long l;
};

void f(struct A a, struct A b)
#ifdef IMPL
{
	int printf(const char *, ...);

	printf("%d %d %ld, %d %d %ld\n",
			a.i, a.j, a.l,
			b.i, b.j, b.l);
}
#else
	;
#endif

#ifndef IMPL
int main()
{
	struct A b = { 1, 2, 3 };

	f((struct A){ 5, 2, 6 }, b);
}
#endif
