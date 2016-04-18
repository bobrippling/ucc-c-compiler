// RUN: %ucc -fsyntax-only %s

struct
__attribute((packed))
	A
{
	/* 0-4:   ciii
	 * 4-8:   ippp
	 * 8-12:  p___
	 * 12-16: ____
	 */
	char c;
	int i;
	int *p;
};

#ifdef __UCC__
#  define offsetof(S, m) (unsigned long)&((S *)0)->m
#else
#  define offsetof __builtin_offsetof
#endif

#define assert_offset(s, m, n)        \
	_Static_assert(offsetof(s, m) == n, \
			"offsetof(" #s ", " #m ") != " #n)

assert_offset(struct A, c, 0);
assert_offset(struct A, i, sizeof(char));
assert_offset(struct A, p, sizeof(char) + sizeof(int));

_Static_assert(
		sizeof(struct A)
		==
		sizeof(int)
		+ sizeof(int *)
		+ sizeof(char),
		"");

typedef unsigned short u2;
struct B
{
	// 2, 4, 6, 7
	u2 a, b, c; char d;
	// 8, despite the gap of 1 - don't split across boundaries
	int i;
};

assert_offset(struct B, a, 0);
assert_offset(struct B, b, sizeof(u2));
assert_offset(struct B, c, sizeof(u2) * 2);
assert_offset(struct B, d, sizeof(u2) * 3);
assert_offset(struct B, i, sizeof(u2) * 3 + sizeof(char) + /*round up*/sizeof(char));

_Static_assert(
		sizeof(struct B)
		==
		sizeof(u2) * 3
		+ sizeof(char)
		+ /*round up*/sizeof(char)
		+ sizeof(int),
		"");
