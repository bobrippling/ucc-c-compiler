#ifdef GNU_VARIADIC
#  define FOO(args...)    do { foo(args); } while (0)
#else
#  define FOO(...)    do { foo(__VA_ARGS__); } while (0)
#endif

#define BAR(x)  FOO("x"); FOO(")");

void
foo(char *a)
{
	BAR();
}
