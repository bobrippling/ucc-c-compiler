// RUN: [ `%ucc -fshow-static-asserts -fsyntax-only %s 2>&1|grep 'passed'|wc -l` -eq 9 ]

#define QUOTE_(a) #a
#define QUOTE(a) QUOTE_(a)
#define SA() _Static_assert(1, QUOTE(__COUNTER__))

struct A
{
	SA();
};

struct B
{
	int i;
	SA();
	int j;
};

SA();
main()
{
	SA();
	int i;
	SA();
	f();
	SA();
	int j;
	SA();
	g();
	SA();
}
SA();
