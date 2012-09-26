main()
{
	enum
	{
		A = 5
	};

	struct
	{
		int a : 5 - 3;
		int b : A;
	};
}
