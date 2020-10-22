// RUN: %ocheck 1 %s
void exit(int) __attribute((noreturn));

f(_Bool x)
{
	exit(x);
}

main()
{
	int x = 3 - (2 + 1);
	f(0 == x);
}
