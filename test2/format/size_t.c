// RUN: %check %s

int printf(const char *, ...)
	__attribute((format(printf, 1, 2)));

typedef unsigned long size_t;

void f()
{
	printf("%zu\n", (size_t)0); // CHECK: !/warn/
	printf("%zd\n", (size_t)0); // CHECK: !/warn/
	printf("%zzzf", 5.2f); // CHECK: warning: unexpected printf modifier 'z' for %f

	printf("%lu\n", (signed long)0); // CHECK: !/warn/
	printf("%ld\n", (unsigned long)0); // CHECK: !/warn/
}
