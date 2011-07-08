#include <stdio.h>

#include "tokenise.h"
#include "util.h"
#include "parse.h"

int main(int argc, char **argv)
{
	FILE *f;

	if(argc != 2)
		die("Usage: %s file", *argv);

	f = fopen(argv[1], "r");
	if(!f)
		die("open %s:", argv[1]);

	tokenise_set_file(f, argv[1]);
	parse();

	return 0;
}
