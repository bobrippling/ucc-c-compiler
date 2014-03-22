// RUN: %ucc %s
// RUN: %check %s

main()
{
	static int i;

	__builtin_constant_p( // CHECK: /warning: unused expression/
			__builtin_constant_p(
				__builtin_types_compatible_p(int, __typeof(i))));

	__builtin_trap();
}
