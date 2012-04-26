main()
{
	int (*(*a)(int))(void);
	int (*(*(x[4]))(int))(void); // = { a, 0, 0, 0 };
	x[0](2)();
}
