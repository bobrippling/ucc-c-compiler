typedef struct A {
	int i, j;
} A;

f(A);

main()
{
	A a = { 1, 2 };
	f(a);
}
