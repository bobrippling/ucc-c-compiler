main()
{
	struct
	{
		int : 2;
	} a; // stack space of two bytes

	struct
	{
		int f1 : 3;
		int : 2;
		int f2 : 1;
		int : 0;
		int f3 : 5;
		int f4 : 7;
		unsigned int f5 : 7;
	} b;
}
