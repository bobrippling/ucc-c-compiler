#include <stdio.h>

int main(int argc, char **argv)
{
	FILE *f;
	int i;

	if(argc < 3){
		fprintf(stderr, "Usage: %s file text...\n", *argv);
		return 1;
	}

	f = fopen(argv[1], "w");

	if(!f){
		perror("open()");
		return 1;
	}

	for(i = 2; i < argc; i++)
		fprintf(f, "%s\n", argv[i]);

	fclose(f);
	return 0;
}
