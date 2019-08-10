// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

typedef struct
{
	int k;
	int l;
	int a[2];
} T;

typedef struct
{
	T t;
	int i;
} S;

main()
{
	T t = { 1, 2, { 3, 4 } };

	S s = { .t = t, .t.k = 1 };

	if(t.k != 1
	|| t.l != 2
	|| t.a[0] != 3
	|| t.a[1] != 4)
		abort();

	if(s.t.k != 1
	|| s.t.l != 0
	|| s.t.a[0] != 0
	|| s.t.a[1] != 0
	|| s.i != 0)
		abort();

	return 0;
}
