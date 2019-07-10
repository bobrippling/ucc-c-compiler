// RUN: %ucc -o %t %s -g
void exit(int) __attribute((noreturn));

f(int *p)
{
	exit(*p);
}

main()
{
	int hello = 2;

	{
		int scope_entry = 5;

		f(&scope_entry);

		__builtin_trap();
	}

	g(); // no undefined ref - DCE'd

	return hello;
}
