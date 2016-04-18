// RUN: %check --only %s

int printf(const char *, ...)
	__attribute((format(printf, 1, 2)));

main()
{
	printf("%d %d\n", // CHECK: warning: %d expects a 'int' argument, not 'unsigned long'
	// CHECK: ^warning: %d expects a 'int' argument, not 'unsigned long'
			sizeof(0), // CHECK: note: argument here
			sizeof(0)); // CHECK: note: argument here
}
