// RUN: %ocheck 0 %s

#define JOIN_(a, b) a ## b
#define JOIN(a, b) JOIN_(a, b)

#define INIT(p)               \
__attribute((constructor(p))) \
static void JOIN(f_, __LINE__)(void)

struct A
{
	int a, b, c;
};

const struct A final = { 5, 7, 3 };
struct A init = {};

static void check(const struct A *expect)
{
	if(init.a != expect->a
	|| init.b != expect->b
	|| init.c != expect->c)
	{
		_Noreturn void abort();
		abort();
	}
}

INIT(9)
{
	check(&(struct A){ .a = final.a });

	init.b = final.b;
}

INIT(1)
{
	check(&(struct A){});

	init.a = final.a;
}

INIT()
{
	check(&(struct A){ .a = final.a, .b = final.b });

	init.c = final.c;
}

main()
{
	check(&final);

	return 0;
}
