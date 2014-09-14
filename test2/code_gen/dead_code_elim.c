// RUN: %ocheck 3 %s

g()
{
	return 3;
}

main()
{
	if(0){
		f(); // shouldn't hit a linker error here - dead code
a:
		return g();
	}

	goto a;
}
