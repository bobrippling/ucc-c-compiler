// RUN: %ocheck 2 %s

struct A
{
	int i;
};

f(int i, struct A *p)
{
	return (i ? p : 0)->i
	+ (i ? 0 : p)->i
	+ (i ? p : (void *)0)->i
	+ (i ? (void *)0 : p)->i;
}

g(int i, struct A *p)
{
	struct A b = *p;
	return (i ? p : &b)->i + (i ? &b : p)->i;
}

main()
{
	return g(1, (struct A []){ 1, 2 });
}
