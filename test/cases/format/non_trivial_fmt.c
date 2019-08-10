// RUN: %check --only %s

void f(int a, int b)
{
	int printf(char *, ...) __attribute((format(printf, 1, 2)));
	printf(a ? "b=%d (a=%s)\n" : "b=%d\n", b, a); // CHECK: warning: %s expects a 'char *' argument, not 'int'
	// CHECK: ^warning: too many arguments for format
}
