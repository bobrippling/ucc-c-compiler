// RUN: %ocheck 0 %s
// RUN: %layout_check %s

struct __attribute((packed)) A
{
	char c;
	short s;
	int i;
};

struct A a = { 1, 2, 3 };

void a_equal(struct A *a, struct A *b)
{
	void abort(void);

	if(a->c != b->c)
		abort();
	if(a->s != b->s)
		abort();
	if(a->i != b->i)
		abort();
}

main()
{
	struct A b = { 1, 2, 3 };
	struct A c = { .i = 3, .c = 1, .s = 2 };

	a_equal(&a, &b);
	a_equal(&a, &c);
	a_equal(&b, &c);

	return 0;
}
