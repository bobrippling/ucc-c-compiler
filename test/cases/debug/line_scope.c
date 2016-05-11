// RUN: %debug_scope %s | %stdoutcheck %s

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

// STDOUT: arg 3 12
// STDOUT: i 5 10
// STDOUT: ret 3 12
