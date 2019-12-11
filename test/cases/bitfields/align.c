// RUN: %ucc -fsyntax-only %s

struct A {
	char a;
	unsigned b : 2;
	char c;
} x;

_Static_assert(sizeof(x) == _Alignof(unsigned));
_Static_assert(_Alignof(x) == _Alignof(unsigned));
_Static_assert(__builtin_offsetof(struct A, c) == 2);

_Static_assert(sizeof(x) == __alignof__(unsigned));
_Static_assert(__alignof__(x) == __alignof__(unsigned));
_Static_assert((unsigned long)(&((struct A*)0)->c) == 2);
