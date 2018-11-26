// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_fail

void san_fail(void)
{
	extern _Noreturn void exit(int);
	exit(5);
}

f(int i)
{
	char buf[i];
	buf[0] = 5;
	return buf[0];
}

main()
{
	f(0);
}
