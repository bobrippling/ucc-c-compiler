// RUN: %check --only %s

struct A
{
	int i;
};

void take(void *);

void f(const void *p)
{
	struct A *a = p; // CHECK: warning: implicit cast removes qualifiers (const)
	struct A *b = (struct A *)p;

	(void)a;
	(void)b;

	const char c = 5;
	take(&c); // CHECK: warning: implicit cast removes qualifiers (const)

	// shouldn't warn for these either:
	char c2;
	(void)(&c == 0);         // cast    int --> const char *
	(void)(&c2 == 0);        // cast    int -->       char *
	(void)(&c == (void*)0);  // cast void * --> const char *
	(void)(&c2 == (void*)0); // cast void * -->       char *

	(void)(&c == &(int){3}); // CHECK: warning: distinct pointer types in comparison lacks a cast
}
