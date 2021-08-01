// RUN: %ucc -fsyntax-only %s

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

#define offsetof(s, m) (unsigned long)&((struct s *)0)->m
#define CHK(val, expected) _Static_assert(val == expected, "")

CHK(offsetof(Y, i), 0);
CHK(offsetof(Y, j), 4);
CHK(offsetof(Y, k), 8);
CHK(sizeof(struct Y), 12);

CHK(offsetof(Z, p), 0);
CHK(sizeof(struct Z), 4);

CHK(offsetof(X, a), 0);
CHK(offsetof(X, middle), 12);
CHK(offsetof(X, b), 16);
CHK(sizeof(struct X), 20);
