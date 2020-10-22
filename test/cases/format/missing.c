// RUN: %check --only %s

int printf(char *, ...)
	__attribute((format(printf, 1, 2)));

void f()
{
	printf("%z", 2); // CHECK: warning: incomplete format specifier
}
