// RUN: %check %s
void *memset(void *b, int c, unsigned long len);
void *memmove(void *, const void *, unsigned long);
void *memcpy(void *, const void *, unsigned long);
int memcmp(const void *, const void *, unsigned long);

struct A
{
	int i, j;
};

void good(struct A *a, const struct A *b)
{
	memcmp(a, b, sizeof *a); // CHECK: !/warning: mem/
	memset(a, 0, sizeof *a); // CHECK: !/warning: mem/
	memcpy(a, b, sizeof *a); // CHECK: !/warning: mem/
	memmove(a, b, sizeof *a); // CHECK: !/warning: mem/
}

void bad(struct A *a, struct A *b)
{
	memcmp(a, b, sizeof a);  // CHECK: warning: memcmp with different types 'struct A' and 'struct A *'
	memset(a, 0, sizeof a);  // CHECK: warning: memset with different types 'struct A' and 'struct A *'
	memcpy(a, b, sizeof a);  // CHECK: warning: memcpy with different types 'struct A' and 'struct A *'
	memmove(a, b, sizeof a); // CHECK: warning: memmove with different types 'struct A' and 'struct A *'
}
