// RUN: %ocheck 3 %s -fno-semantic-interposition
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

__attribute((always_inline))
f()
{
	goto a;
a:
	return 3;
}

main()
{
	goto a;
b:
	printf("hi\n");
	return f();

a:
	goto b;
}
