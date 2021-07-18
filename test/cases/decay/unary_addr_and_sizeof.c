// RUN: %ucc -o %t %s
// RUN: %t; [ $? -eq 1 ]

f()
{
}

main()
{
	f(&main); // main shouldn't decay, i.e. &(int(*)())main

	return sizeof(main); // main shouldn't decay, i.e. sizeof( (int(*)())main )
}
