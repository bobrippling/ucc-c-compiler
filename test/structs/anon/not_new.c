// RUN: %ucc -fsyntax-only %s

main()
{
	struct A
	{
		int a;
	};

	{
		struct B
		{
			struct A;
			/* struct A; here is not making a new type
			 * we just need a parse success (no errors about incomplete types)
			 */
			int b;
		} b;
	}
}
