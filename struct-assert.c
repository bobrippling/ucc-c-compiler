//#include <stddef.h>
//#define offsetof(t, m) (unsigned long)(&((t *)0)->m)

/*
#ifdef __clang__
#  warning clang static assert
#  define JOIN_(x, y) x ## y
#  define JOIN(x, y) JOIN_(x, y)
#  define _Static_assert(x, s) char JOIN(__yo, __LINE__)[(x) && s ? 1 : -1]
#endif
*/

//#define OFF 4
struct __attribute__((/*packed*/)) B
{
	char a;
	int i;
};

struct A
{
	struct B b;
	_Static_assert(
			//offsetof(struct B, i) == 1,
			(unsigned long)(&((struct B *)0)->i) == 4, // NOT CONST
			"S not packed");
};

int glob;

/* &glob == &glob isn't an integer constant expression,
 * but is an address constant expression, so may be used
 * in an initialiser. as an extension we allow it as an
 * integer constant expression */
_Static_assert(&glob == &glob, ""); // !cosnt

_Static_assert(&glob != 0, "");

_Static_assert(
		&((int *)0)[5] == 5 * sizeof(int),
		"");

main()
{
	if((unsigned long)(&((struct B *)0)->i) != 4)
		abort();

	_Static_assert(
			(unsigned long)(&((struct B *)0)->i) // !const
			==
			(unsigned long)(&((struct B *)0)->i),
			"");

	_Static_assert(
			&((struct B *)0)->i == &((struct B *)0)->i, // !const
			"");
}
