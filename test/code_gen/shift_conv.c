// RUN: %archgen %s 'x86,x86_64:shll $1, %%eax' 'x86,x86_64:!/addl.*%%eax'
// RUN: %ocheck 0 %s

f(unsigned i)
{
	return 2 * i;
}

g(int i)
{
	return i / 2;
}

main()
{
	if(f(2) != 4)
		abort();

	if(g(-3) != -1)
		abort();

	return 0;
}
