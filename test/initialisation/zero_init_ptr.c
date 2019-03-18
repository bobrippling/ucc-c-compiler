// RUN: %ocheck 0 %s

struct A
{
	int before;
	const char *p;
	int after;
};

const char *f(int *pb, int *pa)
{
	*pb = 5;
	*pa = 8;
	return "hi";
}

int strcmp(const char *, const char *);
_Noreturn void abort(void);

main()
{
	struct A a = {
		.p = f(&a.before, &a.after)
	};

	if(strcmp(a.p, "hi"))
		abort();

	if(a.after != 0)
		abort();

	if(a.before != 5)
		abort();

	return 0;
}
