#include <stdatomic.h>
int printf(const char *, ...);

int *_Atomic p;

int main()
{
	int a[12];

	p = a;
	//p += 4; // p -> &a[4]

	printf("%zd\n", p - a);

	p = a;
	atomic_fetch_add(&p, 4); // p -> &a[1]? &a[4]?
	// ^ this is UB

	printf("%zd\n", p - a);
}
