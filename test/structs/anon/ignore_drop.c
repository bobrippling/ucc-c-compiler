// RUN: %check -e %s

main()
{
	struct A
	{
		int a;
	};

	{
		struct B
		{
			// the anonymous struct A shouldn't participate in initialisation

			struct A; // CHECK: warning: unnamed member 'struct A' ignored (untagged would be accepted in C11)
			int b;
		} b = { 1 };

		return b.a; // CHECK: error: struct B has no member named "a"
	}
