#include <stdio.h>
#include <string.h>

#include "tree.h"
#include "tokenise.h"
#include "util.h"
#include "parse.h"
#include "fold.h"
#include "gen_asm.h"
#include "gen_str.h"
#include "sym.h"

int main(int argc, char **argv)
{
	static global **globs;
	void (*gf)(global **);
	FILE *f;
	const char *fname;
	int i;

	gf = gen_asm;
	fname = NULL;

	for(i = 1; i < argc; i++){
		if(!strcmp(argv[i], "-X")){
			if(++i == argc)
				goto usage;

			if(!strcmp(argv[i], "print"))
				gf = gen_str;
			else if(!strcmp(argv[i], "asm"))
				gf = gen_asm;
			else
				goto usage;

		}else if(!fname){
			fname = argv[i];
		}else{
usage:
			die("Usage: %s [-X backend] file", *argv);
		}
	}

	if(fname){
		f = fopen(fname, "r");
		if(!f){
			if(strcmp(fname, "-"))
				die("open %s:", fname);
			goto use_stdin;
		}
	}else{
use_stdin:
		f = stdin;
		fname = "-";
	}

	tokenise_set_file(f, fname);
	globs = parse();
	if(f != stdin)
		fclose(f);

	fold(&globs);
	gf(globs);

	return 0;
}
