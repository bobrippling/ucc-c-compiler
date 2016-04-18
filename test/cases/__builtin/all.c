// RUN: %ucc -o %t %s -std=c89
// C89 for implicit function decl
// RUN: %t

#define ASSERT_IS(n, exp) _Static_assert(n == (exp), #exp " failed")

not_builtin()
{
}

main()
{
	typedef int *intp;

	goto next;
	__builtin_trap();
next:

	not_builtin();

	// 1
	ASSERT_IS(1, __builtin_constant_p(__builtin_constant_p(5)));

	// 0
	ASSERT_IS(1, __builtin_constant_p(__builtin_types_compatible_p(void, int)));

	// 3
	ASSERT_IS(1, __builtin_types_compatible_p(int *, intp));
	ASSERT_IS(1, __builtin_types_compatible_p(int *, __typeof(0) *));
	ASSERT_IS(0, __builtin_constant_p(f()));
	ASSERT_IS(1, __builtin_constant_p(1));

	return 0;
	__builtin_unreachable();
}
