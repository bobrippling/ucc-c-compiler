// RUN: %debug_check %s

int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

int f(int arg)
{
	int ret = 5 + arg;
	for(int i = 0; i < 3; i++){
		printf("%d\n", i);
	}
	return ret;
}

main()
{
	return f(2);
}
