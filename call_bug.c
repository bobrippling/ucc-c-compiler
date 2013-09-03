a(){}

call(int idx)
{
	static void (*fns[])() = {
		a,
	};

	fns[idx]();
}

main()
{
	call(0);
}
