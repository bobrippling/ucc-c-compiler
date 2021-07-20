//#include <stdlib.h>
//#include <stdio.h>

//#define alloca __builtin_alloca

int putchar(int);

void f() {
	void g(void *);

	char *space = __builtin_alloca(5);

	*space = 'h';

	g(space);
}

/*
int main(void) {
	const int N = 26;
	char *(p[N]);

	for(int i = 0; i < N; ++i)
		*(p[i] = __builtin_alloca(1)) = 'a' + i;

	for(int i = 0; i < N; ++i)
		putchar(*(p[i]));

	return 0;
}
*/
