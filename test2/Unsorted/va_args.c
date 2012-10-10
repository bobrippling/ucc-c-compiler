#include <stdio.h>

#define printf_1(fmt, ...) printf(fmt, __VA_ARGS__)
#define printf_proper(...) printf(__VA_ARGS__)

main()
{
	printf_1("five = %d\n", 5);

	printf_proper("hello\n");
	printf_proper("%d %s\n", 2, "yo");
}
