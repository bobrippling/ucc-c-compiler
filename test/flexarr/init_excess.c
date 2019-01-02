// RUN: %check %s

main()
{
	static struct
	{
		int n;
		struct
		{
			char *a;
			int z;
		} x[];
	} abc = {
		2,
		{ "hello" },
		{ "yo" } // CHECK: /warning: excess init/
	};
}
