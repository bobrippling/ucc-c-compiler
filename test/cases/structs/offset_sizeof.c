// RUN: %ucc %s -o %t
// RUN: %t | diff -u - %s.ocheck
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

struct X
{
	struct Y
	{
		int i, j, k;
	} a;

	int middle;

	struct Z
	{
		int p;
	} b;
};

main()
{
#include "../ocheck-init.c"
#define offsetof(s, m) (unsigned long)&((struct s *)0)->m
#define P(x) printf(#x " = %u\n", x)

	P(offsetof(Y, i));
	P(offsetof(Y, j));
	P(offsetof(Y, k));
	P(sizeof(struct Y));

	P(offsetof(Z, p));
	P(sizeof(struct Z));

	P(offsetof(X, a));
	P(offsetof(X, middle));
	P(offsetof(X, b));
	P(sizeof(struct X));
}
