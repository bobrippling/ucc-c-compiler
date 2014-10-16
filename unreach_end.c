main()
{
	{
		int scope_entry = 5;

		f(&scope_entry);

		__builtin_trap();
	}
}
