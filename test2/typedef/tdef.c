// RUN: %check -DCLASH -e %s
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
		typedef signed t;   // CHECK: /warning: shadowing definition of t/
#ifdef CLASH
		t t; // CHECK: /error: clashing definitions/
#endif

		yo a = 2;

		r = a;
	}

	(void)(a.t + b.t + c.t + d.t + e.t + e.i);

	return r;
}
