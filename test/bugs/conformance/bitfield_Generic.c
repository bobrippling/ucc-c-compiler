typedef unsigned long size_t;

enum { false, true };
typedef _Bool bool;

typedef struct A
{
	size_t a : 63;
	bool b : 1;
} A;

int printf(const char *, ...)
	__attribute__((format(printf, 1, 2)));

static void init(A *p, size_t n, bool x)
{
	p->a = n;
	p->b = x;
}

int main()
{
	A a;

	init(&a, 10, false);

  (void)_Generic(a.a, size_t: 1);
  (void)_Generic(a.b, bool: 1);
  _Static_assert(
      __builtin_types_compatible_p(
        __typeof(((void)0, a.a)),
        size_t),
      "");
  _Static_assert(
      __builtin_types_compatible_p(
        __typeof(((void)0, a.b)),
        bool),
      "");

	printf("%zu %s\n", a.a, a.b ? "true" : "false");

	a.b = !a.b;

	printf("%zu %s\n", a.a, a.b ? "true" : "false");

	a.a++;

	printf("%zu %s\n", a.a, a.b ? "true" : "false");

	a.b = true;
	printf("%zu %s\n", a.a, a.b ? "true" : "false");
}
