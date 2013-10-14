main()
{
	int i = 0; // CHECK: /note: local declaration here/

	f(i);

	{
		int i = 0; // CHECK: /warning: declaration of "i" shadows local declaration/

		f(i);
	}
}
