#include <stdio.h>
#include <stdlib.h>

void bad_input(const char *desc)
{
	fprintf(stderr, "bad input (%s)\n", desc);
	exit(1);
}

int plus(int a, int b)
{
	return a + b;
}

int minus(int a, int b)
{
	return a - b;
}

int (*getptr(char ch))()
{
	switch(ch){
		case '+':
			return plus;
		case '-':
			return &minus;
	}
	bad_input("need + or -");
}

int main(int argc, char **argv)
{
	int a, b;
	int (*f)(int, int);

	if(argc != 2 || argv[1][1])
		bad_input("arglen");

	a = 3;
	b = 2;
	f = getptr(argv[1][0]);

	printf("%d %c %d = %d\n", a, argv[1][0], b, f(a, b));
	return 0;
}
