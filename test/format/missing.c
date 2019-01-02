// RUN: %check %s

int printf(char *, ...)
	__attribute((format(printf, 1, 2)));

f()
{
	printf("%z", 2); // CHECK: warning: missing conversion character
}
