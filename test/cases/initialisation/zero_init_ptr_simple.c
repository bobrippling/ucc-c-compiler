// RUN: %ocheck 0 %s

struct A
{
	const char *p;
	int n;
};

const char *f(int *p)
{
	*p = 3;
	return "hi";
}

int strcmp(const char *, const char *);
_Noreturn void abort(void);

main()
{
	struct A a = {
		f(&a.n)
	};

	if(strcmp(a.p, "hi"))
		abort();
	if(a.n != 0)
		abort();

	return 0;
}
