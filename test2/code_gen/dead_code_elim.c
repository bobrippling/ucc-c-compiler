// RUN: %ocheck 3 %s -g
// test debug emission too

g()
{
	return 3;
}

main()
{
	if(0){
		int i;
		f(); // shouldn't hit a linker error here - dead code
a:
		i = 2;
		return g(i);
	}

	goto a;
}
