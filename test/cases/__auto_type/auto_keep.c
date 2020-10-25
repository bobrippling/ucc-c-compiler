// RUN: %check -e %s

// check alignment, const and attribute preservation

_Alignas(long) __auto_type yo = (char)3;

_Static_assert(_Alignof(yo) == _Alignof(long), "yo alignment");

__attribute__((__ucc_debug)) // CHECK: warning: debug attribute handled
__auto_type attributed = "hi";

main()
{
	const __auto_type k = 2;

	k = 3; // CHECK: error: can't modify const
}
