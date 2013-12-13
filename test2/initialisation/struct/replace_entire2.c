// RUN: %ocheck 0 %s

struct A
{
	struct B
	{
		int i, j;
	} b;
	int k;
};

typedef struct {
	int k;
	int l;
	int a[2];
} T;

typedef struct {
	T t;
	int i;
} S;

f()
{
	T t = { 1, 2, { 3, 4 } };

	S s = { .t = t, .t.k = 1 };

	if(s.t.k != 1 || s.t.l != 0
	|| s.t.a[0] != 0
	|| s.t.a[1] != 0)
		abort();
	if(s.i != 0)
		abort();
}

main()
{
	f();
	return 0;
}
