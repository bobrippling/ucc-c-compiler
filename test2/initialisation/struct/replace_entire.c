// RUN: %ocheck 0 %s

struct T
{
	int a;
	int b;
};

struct S
{
	struct T t;
};

f()
{
	struct T t = { 1, 2 };

	struct S s = {
		.t = t,
		.t.b = 41, // wipes previous init
	};

	return t.a;
}

main()
{
	return f();
}
