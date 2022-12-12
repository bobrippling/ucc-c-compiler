// RUN: %ocheck 0 %s
void abort(void) __attribute__((noreturn));

struct B
{
	char a;
	int i;
};

struct A
{
	struct B b;
	_Static_assert(
			(unsigned long)(&((struct B *)0)->i) == 4,
			"S not packed");
};

int glob;

/* &glob == &glob isn't an integer constant expression,
 * but is an address constant expression, so may be used
 * in an initialiser. as an extension we allow it as an
 * integer constant expression */
_Static_assert(&glob == &glob, "");

_Static_assert(&glob != 0, "");

_Static_assert(
		&((int *)0)[5] == 5 * sizeof(int),
		"");

main()
{
#include "../ocheck-init.c"
	if((unsigned long)(&((struct B *)0)->i) != 4)
		abort();

	_Static_assert(
			(unsigned long)(&((struct B *)0)->i)
			==
			(unsigned long)(&((struct B *)0)->i),
			"");

	_Static_assert(
			&((struct B *)0)->i == &((struct B *)0)->i,
			"");

	return 0;
}
