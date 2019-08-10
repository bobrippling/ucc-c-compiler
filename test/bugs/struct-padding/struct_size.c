#include <stdint.h>

struct data
{
	unsigned int foo:4;
	unsigned int bar:4;
	uint8_t other;
	uint16_t other2;
};

int main(void)
{
	return sizeof(struct data);
}

/*
	 $ gcc -Wall -Werror -Wextra -g -O2 -o test_gcc test.c
	 $ ./test_gcc ; echo $?
	 4
	 $ clang -Wall -Werror -Wextra -g -O2 -o test_clang test.c
	 $ ./test_clang ; echo $?
	 4
	 $ tcc -Wall -Werror -Wextra -g -O2 -o test_tcc test.c
	 $ ./test_tcc ; echo $?
	 8
	 */

#ifdef MORE
struct Basic
{
	int x : 4, y : 4;
} bas = { 1, 2 };

_Static_assert(sizeof(bas) == sizeof(int), "");

struct Basic2
{
	char x : 4, y : 4;
} bas2 = { 1, 2 };

_Static_assert(sizeof(bas2) == sizeof(char), "");

int main()
{
	printf("bas=%p\n", &bas);
	printf("bas2=%p\n", &bas2);
	printf("diff = %zd\n", (char*)&bas2 - (char*)&bas);
}
#endif
