#include <assert.h>

int main(int argc, char **argv)
{
	assert(argc == 2);
	printf("argv[1] = %s\n", argv[1]);
	return 0;
}
