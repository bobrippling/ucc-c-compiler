// RUN: %ucc -c %s -o %t

f();

main()
{
	const int x = 0;

	if(!!f() != x)
		;
}
