#include <stdio.h>
#include <string.h>

#include "tree.h"
#include "tokenise.h"
#include "util.h"
#include "parse.h"
#include "fold.h"
#include "gen.h"

int main(int argc, char **argv)
{
	static function **prog;
	FILE *f;

	if(argc > 2)
		die("Usage: %s file", *argv);

	if(argc == 2){
		f = fopen(argv[1], "r");
		if(!f){
			if(strcmp(argv[1], "-"))
				die("open %s:", argv[1]);
			f = stdin;
		}
	}else{
		f = stdin;
	}

	tokenise_set_file(f, argv[1]);
	prog = parse();
	if(f != stdin)
		fclose(f);
	fold(prog);
	gen(prog);

	return 0;
}
