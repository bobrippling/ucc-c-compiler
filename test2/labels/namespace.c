// RUN: %ocheck 0 %s

f()
{
	goto a;
a:;
}

main()
{
	goto a;
a: f();
	 return 0;
}
