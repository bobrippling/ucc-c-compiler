#include <stdio.h>

extern void* apply(void* func, void** args, size_t argc);

/*
long bar(int a, int b, long c, long d, int e, long f, long g, int h) {
	printf( "a = %x\n"
			"b = %x\n"
			"c = %lx\n"
			"d = %lx\n"
			"e = %x\n"
			"f = %lx\n"
			"g = %lx\n"
			"h = %x\n",
			a, b, c, d, e, f, g, h);
	return h;
}

long foo(long a, long b, long c) {
	long baz = a ^ b & c;
	printf("a ^ b & c = %d\n", baz);

	long args[] = {
		0xaaddaadd,
		0xddaaddaa,
		0xaaaaaaaabbbbbbbb,
		0x1234123412341234,
		1,
		0x4321432143214321,
		0x9879879879879879,
		2
	};
	void* result = apply(bar, (void**) &args, 8);
	printf("--> %ld\n", (long) result);
	return (long) result;
}
*/

long f(long i)
{
	return i + 1;
}

long call(long (*f)(long), long arg)
{
	return f(arg);
}

int main() {
	/*long args[] = {3, 4, 5};*/
	/*void* result = apply(foo, (void**) &args, 3);*/
	/*printf("-> %ld\n", (long) result);*/

	long args[] = {
		(long)f,
		3
	};

	printf("%d\n", (long)apply(call, (void **)args, 2));

	return 0;
}

/*
$ ./test
a ^ b & c = 7
a = aaddaadd
b = ddaaddaa
c = aaaaaaaabbbbbbbb
d = 1234123412341234
e = 1
f = 4321432143214321
g = 9879879879879879
h = 2
--> 2
-> 2
*/


