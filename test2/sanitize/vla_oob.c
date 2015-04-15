// RUN: %ocheck 5 %s -fsanitize=undefined -fsanitize-error=call=san_fail

void san_fail(void)
{
	extern _Noreturn void exit(int);
	exit(5);
}

f(int i)
{
	short buf[i];

	return buf[3];
}

main()
{
	f(3);
}
