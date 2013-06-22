// RUN: [ `%ucc -fsyntax-only %s 2>&1|grep 'passed'|wc -l` -eq 9 ]

struct A
{
	_Static_assert(1, "");
};

struct B
{
	int i;
	_Static_assert(1, "");
	int j;
};

_Static_assert(1, "");
main()
{
	_Static_assert(1, "");
	int i;
	_Static_assert(1, "");
	f();
	_Static_assert(1, "");
	int j;
	_Static_assert(1, "");
	g();
	_Static_assert(1, "");
}
_Static_assert(1, "");
