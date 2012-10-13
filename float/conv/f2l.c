#include <stdio.h>

int main(int argc, char **argv)
{
	union
	{
		long l;
		double f;
	} a;

	if(argc != 2){
usage:
		fprintf(stderr, "Usage: %s fp\n", *argv);
		return 1;
	}

	if(sscanf(argv[1], "%lf", &a.f) != 1)
		goto usage;

	printf("%ld\n", a.l);

	return 0;
}
