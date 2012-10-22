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
	__builtin_constant_p(__builtin_constant_p(5));

	// 0
	__builtin_constant_p(__builtin_types_compatible_p(void, int));

	// 3
	return __builtin_types_compatible_p(int *, intp)
		   + __builtin_types_compatible_p(int *, typeof(0) *)
			 + __builtin_constant_p(f())
			 + __builtin_constant_p(1);

	__builtin_unreachable();
}
