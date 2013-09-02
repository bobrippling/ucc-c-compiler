main()
{
	typedef int int_t;
	{
		int int_t = 5; // CHECK: /warning: declaration of "int_t" shadows local declaration/
	}
}
