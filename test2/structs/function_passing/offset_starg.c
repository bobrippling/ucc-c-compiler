// RUN: %ocheck 0 %s

typedef struct
{
	int i, j;
} A;

void f(int i, A a)
{
	if(i != 5)
		abort();
	if(a.i != 1)
		abort();
	if(a.j != 2)
		abort();
}

main()
{
	f(5, (A){ 1, 2 });

	return 0;
}
