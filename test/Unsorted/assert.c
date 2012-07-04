#include <assert.h>

int main(int argc, char **argv)
{
	assert(argc == 1);
	printf("argv[0] = %s\n", argv[0]);
	return 0;
}
