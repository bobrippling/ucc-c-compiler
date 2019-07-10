// RUN: %ocheck 0 %s

struct A
{
	unsigned : 0;
};

f(void *a, void *b)
{
	if(a != b){
		_Noreturn void abort();
		abort();
	}
}

main()
{
	struct A x[10];

	f(&x[2], &x[5]);

	return sizeof(x);
}

