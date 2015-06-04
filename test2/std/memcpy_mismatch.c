// RUN: %check %s

typedef struct A
{
	int i, j;
} A;

typedef __typeof(sizeof(0)) size_t;
void *memcpy(void *, const void *, size_t);

f(A *a, A *b)
{
	memcpy(a, b, sizeof a); // CHECK: warning: memcpy with different types 'A (aka 'struct A')' and 'A (aka 'struct A') *'
	memcpy(a, b, sizeof *a);

	memcpy(a, b, 2 * sizeof a); // CHECK: warning: memcpy with different types 'A (aka 'struct A')' and 'A (aka 'struct A') *'
	memcpy(a, b, 2 * sizeof *a);
}
