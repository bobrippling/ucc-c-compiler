// RUN: %ocheck 1 %s

f()
{
}

main()
{
	f(&main); // main shouldn't decay, i.e. &(int(*)())main

	return sizeof(main); // main shouldn't decay, i.e. sizeof( (int(*)())main )
}
