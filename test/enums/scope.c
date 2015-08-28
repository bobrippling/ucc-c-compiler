// RUN: %ucc -fsyntax-only %s

enum {
	A = 12
};

static void f(void)
{
 enum {
		A = A + 1,
		B
	};

 _Static_assert(A == 13, "");
 _Static_assert(B == 14, "");
}

