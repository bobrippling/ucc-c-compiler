typedef struct A { int a, b; } A;

dump(A *p)
{
	printf("{ %d, %d }\n", p->a, p->b);
}

main()
{
	static __auto_type a = (A){ 1, 2 };
	A b;

	b = a;

	dump(&a);
	dump(&b);
}
