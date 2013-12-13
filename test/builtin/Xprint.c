main()
{
	int i = __builtin_constant_p(f());
	int j = __builtin_types_compatible_p(typeof(({ int i; &i; })), typeof(i));

	return i + j;
	__builtin_trap();
	__builtin_unreachable();
}
