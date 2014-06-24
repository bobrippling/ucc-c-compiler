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
