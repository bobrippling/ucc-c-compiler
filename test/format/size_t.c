// RUN: %check --only %s

int printf(const char *, ...)
	__attribute((format(printf, 1, 2)));

typedef unsigned long size_t;

void f()
{
	printf("%zu\n", (size_t)0);
	printf("%zd\n", (size_t)0);
	printf("%zzzf", 5.2f); // CHECK: warning: unexpected printf modifier 'z' for %f

	printf("%lu\n", (signed long)0);
	printf("%ld\n", (unsigned long)0);

	printf("%zu\n", (long long)3);
	printf("%zu\n", (long)3);
}
