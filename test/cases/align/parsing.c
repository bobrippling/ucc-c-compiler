// RUN: %ucc -fsyntax-only %s

_Alignas(int)
struct A
{
	char c;
};

_Static_assert(_Alignof(struct A) == _Alignof(char), "");

_Alignas(int)
typedef struct
{
	char c;
} A;

_Static_assert(_Alignof(A) == _Alignof(char), "");
