struct A
{
	int i, j, k;
	//float f;
};

struct A f(void)
#ifdef IMPL
{
	return (struct A){ 1, 2 };
}
#else
;

int main()
{
	struct A x = f();

	printf("%d %d %d\n", x.i, x.j, x.k);
}
#endif
