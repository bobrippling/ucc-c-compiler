// RUN: %ocheck 0 %s

_Noreturn void exit(int);
void g(int i)
{
}

int f(int p)
{
	(p == 5 ? exit : g)(2);

	// this shouldn't be thought of as unreachable
	return 7;
}

main()
{
	f(4);
	return 0;
}
