// RUN: %ucc -fsyntax-only %s

__attribute((aligned(4))) // applies only to decls, not the struct
struct A
{
	char c;
} a; // aligned 4 from decl attr

struct
__attribute((aligned(4))) // applies to the struct
B
{
	char c;
} b; // aligned 4 from struct B attr


struct A a2; // aligned 1 - no attr
struct B b2; // aligned 4 from struct B attr


_Static_assert(_Alignof(a)  == 4, "");
_Static_assert(_Alignof(b)  == 4, "");
_Static_assert(_Alignof(a2) == 1, "");
_Static_assert(_Alignof(b2) == 4, "");



// final check for postfix attribute
struct C
{
	char c;
} c __attribute((aligned(4)));

struct C c2;

_Static_assert(_Alignof(c)  == 4, "");
_Static_assert(_Alignof(c2) == 1, "");
