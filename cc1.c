#include <stdio.h>

#include "tokenise.h"
#include "util.h"
#include "tree.h"
#include "parse.h"
#include "gen.h"

int main(int argc, char **argv)
{
	FILE *f;
	function **prog;

	if(argc != 2)
		die("Usage: %s file", *argv);

	f = fopen(argv[1], "r");
	if(!f)
		die("open %s:", argv[1]);

	tokenise_set_file(f, argv[1]);
	prog = parse();

	gen(prog);

	return 0;
}
