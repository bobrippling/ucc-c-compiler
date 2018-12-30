// RUN: %ucc -fsyntax-only %s

#define packed __attribute((packed))

struct A
{
	char c;
	packed int i;
};


#define offsetof(S, m) (unsigned long)&((S *)0)->m

#define assert_offset(s, m, n)        \
	_Static_assert(offsetof(s, m) == n, \
			"offsetof(" #s ", " #m ") != " #n)

assert_offset(struct A, c, 0);
assert_offset(struct A, i, 1);
