// see https://gcc.gnu.org/wiki/DebugFission

typedef struct A_s
{
	int i;
	const char *p;
} A;

int f(A *p)
{
	return p->i;
}

int printf(const char *, ...);

int main()
{
	const char *hi = "hi";
	struct A_s a = { 1, hi };

	printf("%d %s\n", a.i, a.p);

	return f(&a);
}
