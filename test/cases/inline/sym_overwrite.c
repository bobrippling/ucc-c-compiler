// RUN: %ucc -o %t %s -finline-functions

int printf(const char *, ...)
	__attribute((format(printf, 1, 2)));

void vla(int n);

void f(int n)
{
	int b = 0; // b sym pushed/popped on inline
	printf("new b @ %p\n", &b);
	vla(n);
}

void vla(int n)
{
	typedef int ar[n]; // ar sym pushed/popped on inline
	printf("new vla, sizeof = %d\n", sizeof(ar));
	f(n);
}

main()
{
	vla(2);
}
