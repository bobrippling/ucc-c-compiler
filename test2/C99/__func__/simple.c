// RUN: %ocheck 0 %s

const char (*f())[]
{
	return &__func__;
}

main()
{
	const char *s = f();
	if(s[0] != 'f' || s[1])
		abort();
	return 0;
}
