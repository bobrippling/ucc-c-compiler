// RUN: %check -e %s

f1()
{
	typedef struct A A; // CHECK: !/error/
	struct A { int j; }; // CHECK: !/error/

	{
		struct A { int i; }; // CHECK: !/error/
		A a = { .j = 2 }; // CHECK: !/error/

		typedef struct A A; // CHECK: !/error/
		A b = { .i = 3 }; // CHECK: !/error/
	}
}

typedef struct A A;

f2()
{
	struct A { int i; }; // CHECK: !/error/

	A b; // CHECK: /error/
}
