#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "../util/util.h"
#include "tree.h"
#include "tokenise.h"
#include "../util/util.h"
#include "parse.h"
#include "fold.h"
#include "gen_asm.h"
#include "gen_str.h"
#include "sym.h"

FILE *cc1_out = NULL;

void ccdie(const char *fmt, ...)
{
	const int i = strlen(fmt);
	va_list l;

	va_start(fmt, l);
	vfprintf(stderr, fmt, l);
	va_end(l);

	if(fmt[i-1] == ':')
		perror(NULL);

	fputc('\n', stderr);
	exit(1);
}

int main(int argc, char **argv)
{
	static symtable *globs;
	void (*gf)(symtable *);
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

		}else if(!strcmp(argv[i], "-o")){
			if(++i == argc)
				goto usage;

			if(strcmp(argv[i], "-")){
				cc1_out = fopen(argv[i], "w");
				if(!cc1_out){
					ccdie("open %s:", argv[i]);
					return 1;
				}
			}

		}else if(!fname){
			fname = argv[i];
		}else{
usage:
			ccdie("Usage: %s [-X backend] file", *argv);
		}
	}

	if(fname){
		f = fopen(fname, "r");
		if(!f){
			if(strcmp(fname, "-"))
				ccdie("open %s:", fname);
			goto use_stdin;
		}
	}else{
use_stdin:
		f = stdin;
		fname = "-";
	}

	if(!cc1_out)
		cc1_out = stdout;

	tokenise_set_file(f, fname);
	globs = parse();
	if(f != stdin)
		fclose(f);

	fold(globs);
	gf(globs);

	fclose(cc1_out);

	return 0;
}
