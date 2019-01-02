// RUN: %ucc -fsyntax-only %s

int f()
{
	int abc = 5;
	{
		enum alphabet
		{
			abc,
			xyz
		};

		_Static_assert(abc == 0, "");
		// enums should be looked up in scope with identifiers, not after
	}
}
