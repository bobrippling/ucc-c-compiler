// RUN: %check --only %s

int printf(const char *, ...)
	__attribute((format(printf, 1, 2)));

typedef unsigned long long size_t;

void f()
{
	printf("%zu\n", (size_t)0);
	printf("%zd\n", (long long)0);
	printf("%zf", 5.2f); // CHECK: warning: invalid length modifier for float format

	printf("%lu\n", (signed long)0); // CHECK: warning: %lu expects a 'unsigned long' argument, not 'long'
	printf("%ld\n", (unsigned long)0); // CHECK: warning: %ld expects a 'long' argument, not 'unsigned long'

	printf("%zu\n", (unsigned long long)3);
	printf("%zu\n", (unsigned long long)3);
}
