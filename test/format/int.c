// RUN: %check %s

int printf(const char *, ...)
	__attribute((format(printf, 1, 2)));

main()
{
	printf("%d %d\n",
			sizeof(0),  // CHECK: warning: format %d expects 'int' argument (got unsigned long)
			sizeof(0)); // CHECK: warning: format %d expects 'int' argument (got unsigned long)
}
