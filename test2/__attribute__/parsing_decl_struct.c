// RUN: %check %s

__attribute((aligned(__alignof(int)))) // CHECK: warning: attribute ignored - no declaration
struct A
{
	char c;
};

_Static_assert(_Alignof(struct A) == _Alignof(char), "");

struct
__attribute((aligned(__alignof(int)))) // CHECK: !/warn/
	B
{
	char c;
};

_Static_assert(_Alignof(struct B) == _Alignof(int), "");



_Static_assert(
		_Alignof(struct __attribute((aligned(__alignof(int)))) { char c; })
		== __alignof(int),
		"");
