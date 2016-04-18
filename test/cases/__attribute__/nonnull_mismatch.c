// RUN: %check -e %s

void fn(int *, int *);
void fn(int *, int *) __attribute__((nonnull(1))); // CHECK: error: mismatching definitions of "fn"
