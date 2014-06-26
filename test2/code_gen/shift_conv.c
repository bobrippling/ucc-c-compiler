// RUN: %archgen %s 'x86,x86_64:shll $1, %%eax' 'x86,x86_64:!/addl.*%%eax'
// RUN: %ocheck 0 %s

f(int i)
{
	return 2 * i;
}

main()
{
	if(f(2) != 4)
		abort();

	return 0;
}
