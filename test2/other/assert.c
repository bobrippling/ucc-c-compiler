// RUN: %ucc -o %t %s
// RUN: %t
// RUN: %t | grep -F 'argv[0] = %t'

#include <assert.h>

int main(int argc, char **argv)
{
	assert(argc == 1);
	printf("argv[0] = %s\n", argv[0]);
	return 0;
}
