// RUN: %ucc -fsyntax-only %s

struct
__attribute__((aligned((__alignof__(struct sockaddr *)))))
	A
{
	unsigned short f;
	char data[128 - sizeof(unsigned short)];
};

_Static_assert( _Alignof(struct A) ==    _Alignof(void *), "");
_Static_assert( _Alignof(struct A) == __alignof__(void *), "");
_Static_assert(__alignof(struct A) ==    _Alignof(void *), "");
_Static_assert(__alignof(struct A) == __alignof__(void *), "");

_Alignas(_Alignof(struct sockaddr *)) // ignored
struct B
{
	unsigned short f;
	char data[128 - sizeof(unsigned short)];
};

_Static_assert( _Alignof(struct B) ==    _Alignof(short), "");
_Static_assert( _Alignof(struct B) == __alignof__(short), "");
_Static_assert(__alignof(struct B) ==    _Alignof(short), "");
_Static_assert(__alignof(struct B) == __alignof__(short), "");
