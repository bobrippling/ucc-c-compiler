main()
{
	struct
	{
		int a;
		struct
		{
			int b, c;
		} sub;
	} st;

	st.sub.b = 5;

	return (&(&st)->sub)->b == 5 ? 0 : 1;
}
