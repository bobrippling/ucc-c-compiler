// align of p is 8:
// RUN: %ucc -fsyntax-only %s

#define assert_align(var, n) \
	_Static_assert(_Alignof(var) == n, #var " alignment not " #n)

char _Alignas(void *) p;

assert_align(p, 8);

// shouldn't error about subsequent smaller alignments
_Alignas(8) _Alignas(32) __attribute((aligned)) _Alignas(4) int x;

main()
{
	int _Alignas(8) i; // 8
	char _Alignas(int) c; // 4

	_Alignas(sizeof(int)) char c2; // +4

	_Alignas(void (*)()) pf; // 8

	_Alignas(8) _Alignas(4) _Alignas(16) int j; // 16

	assert_align(i, 8);
	assert_align(c, 4);
	assert_align(c2, 4);
	assert_align(pf, 8);
	assert_align(j, 16);
}

// ---------------------------------

struct A { char x[5]; };
struct __attribute__((aligned(16))) A16 { char x[5]; };

assert_align(struct A, 1);
assert_align(struct A16, 16);

struct A a __attribute__((aligned(16)));
assert_align(a, 16);
_Static_assert(sizeof(a) == 5, "");


struct A16 a16;
assert_align(a16, 16);
_Static_assert(sizeof(a16) == 16, "");

// ---

struct B { char x[5]; } b __attribute__((aligned(16)));
struct B16 { char x[5]; } b16 __attribute__((aligned(16)));

assert_align(struct B, 1);
assert_align(struct B16, 1);

assert_align(b, 16);
_Static_assert(sizeof(b) == 5, "");

assert_align(b16, 16);
_Static_assert(sizeof(b16) == 5, "");
