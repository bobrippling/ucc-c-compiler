// RUN: %ocheck 5 %s -fsanitize=null -fsanitize-error=call=san_err

void san_err(void)
{
	void exit(int) __attribute((noreturn));
	exit(5);
}

main()
{
	int *p = 0;

	return *p;
}
