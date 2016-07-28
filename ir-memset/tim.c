struct A
{
	int i, j, k;
};

int printf(const char *, ...);

void f(struct A *p)
#ifdef F
{
	printf("%d %d %d\n",
			p->i,
			p->j,
			p->k);
}
#else
;
#endif

#ifdef MAIN
main()
{
	struct A a = { 1, 2, 3 };
	struct A b;

	printf("should be 1,2,3:\n");
	f(&a);
	a = (struct A){};
	printf("should be 0,0,0:\n");
	f(&a);

	b = a;
	printf("should be 0,0,0:\n");
	f(&b);
}
#endif
