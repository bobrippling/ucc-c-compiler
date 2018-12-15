// RUN: %ocheck 11 %s
// RUN: %archgen %s 'x86,x86_64:/movl %%eax, %%ebx/' -fno-inline-functions

g()
{
	return 5;
}

h()
{
	return 6;
}

f(a)
{
	return a;
}

main()
{
	return f(g() + h());
}
