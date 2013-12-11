// RUN: %check %s

typedef unsigned t;

struct
{
	t t;
} a;

struct
{
	short t;
} b;

struct
{
	unsigned t; // CHECK: !/warn/
} c;

struct
{
	const t t;
} d;

struct A
{
	unsigned t;
	const t; // CHECK: /warning: declaration doesn't declare anything/
	t i;
} e;

main()
{
	t r;

	{
		typedef const t yo; // CHECK: !/warn/
		typedef signed t; // empty decl

		yo a = 2;

		r = a;
	}

	return r;
}

chk_members()
{
	(void)(a.t + b.t + c.t + d.t + e.t + e.i);
}
