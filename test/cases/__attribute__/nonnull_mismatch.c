// RUN: %check -e %s

int fn(int *, int *);
int fn(int *, int *) __attribute__((nonnull(1))); // CHECK: error: mismatching definitions of "fn"

/*
int fn(int *a, int *b) __attribute__((nonnull(1)))
{
	if(!a) return 1; // optimiser can remove this
	if(!b) return 2;
	return 3;
}
*/
