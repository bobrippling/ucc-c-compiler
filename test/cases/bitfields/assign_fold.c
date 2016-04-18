// RUN: %ocheck 0 %s

main()
{
	struct
	{
		unsigned a : 3, b : 8, c : 10;
	} s;

	s.a = s.b = s.c = 5678;

	if(s.a != 6
	|| s.b != 46
	|| s.c != 558)
	{
		_Noreturn void abort();
		abort();
	}

	return 0;
}
