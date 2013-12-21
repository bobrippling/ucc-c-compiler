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

			struct A; // CHECK: /warning: unnamed member 'struct A' ignored - tagged/
			int b;
		} b = { 1 };

		return b.a; // CHECK: /error: struct B has no member named "a"/
	}
}
