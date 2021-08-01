// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 1 ]

main()
{
	static int i;

	return __builtin_constant_p(
					__builtin_constant_p(
						__builtin_types_compatible_p(int, __typeof(i))));

	__builtin_trap();
}
