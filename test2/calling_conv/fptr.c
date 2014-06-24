// should both generate _cdecl calls on win32, i.e. push

main()
{
	int (*f)();
	f(1, 2, 3, 4);
	g(1, 2, 3, 4);
}
