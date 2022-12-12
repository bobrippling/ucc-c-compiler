// RUN: %ocheck 0 %s
int printf(const char *, ...) __attribute__((format(printf, 1, 2)));

int glob, glob2;

// unary
int zero = !&glob;
int one = !!&glob;

int eq = &glob == 0;

// binary
int tru = &glob == &glob;
int fals = &glob != &glob;

int tru2 = &glob != &glob2;
int fals2 = &glob == &glob2;

int tru3 = &glob && &glob;
int tru4 = &glob || &glob;

int *p = &tru3 + 2;

int ifp = !1.0;

#define assert(x) yo(#x, x)
yo(char *name, int x)
{
	if(!x)
		printf("%s bad\n", name);
}

main()
{
#include "../ocheck-init.c"
	assert(!zero);
	assert(!fals);
	assert(!fals2);
	assert(!eq);

	assert(one);
	assert(tru);
	assert(tru2);
	assert(tru3);
	assert(tru4);
}
