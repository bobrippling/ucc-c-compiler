#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>

#include <unistd.h>

#include "../util/util.h"
#include "tree.h"
#include "tokenise.h"
#include "../util/util.h"
#include "parse.h"
#include "fold.h"
#include "gen_asm.h"
#include "gen_str.h"
#include "sym.h"
#include "cc1.h"

FILE *cc_out[NUM_SECTIONS];     /* temporary section files */
char  fnames[NUM_SECTIONS][32]; /* duh */
FILE *cc1_out;                  /* final output */

const char *section_names[NUM_SECTIONS] = {
	"text", "data", "bss"
};

void ccdie(const char *fmt, ...)
{
	const int i = strlen(fmt);
	va_list l;

	va_start(l, fmt);
	vfprintf(stderr, fmt, l);
	va_end(l);

	if(fmt[i-1] == ':')
		perror(NULL);

	fputc('\n', stderr);
	exit(1);
}

void io_setup(void)
{
	int i;

	if(!cc1_out)
		cc1_out = stdout;

	for(i = 0; i < NUM_SECTIONS; i++){
		snprintf(fnames[i], sizeof fnames[i], "/tmp/cc1_%d%d", getpid(), i);

		cc_out[i] = fopen(fnames[i], "w+"); /* need to seek */
		if(!cc_out[i])
			ccdie("open \"%s\":", fnames[i]);
	}
}

void io_fin(int do_sections)
{
	int i;

	for(i = 0; i < NUM_SECTIONS; i++){
		/* cat cc_out[i] to cc1_out, with section headers */
		if(do_sections){
			char buf[256];
			long last = ftell(cc_out[i]);

			if(last == -1 || fseek(cc_out[i], 0, SEEK_SET) == -1)
				ccdie("seeking on section file %d:", i);

			fprintf(cc1_out, "section .%s\n", section_names[i]);

			while(fgets(buf, sizeof buf, cc_out[i]))
				if(fputs(buf, cc1_out) <= 0)
					ccdie("write to cc1 output:");

			if(ferror(cc_out[i]))
				ccdie("read from section file %d:", i);
		}
		fclose(cc_out[i]);
		remove(fnames[i]);
	}

	if(fclose(cc1_out))
		ccdie("close cc1 output");
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

	io_setup();

	tokenise_set_file(f, fname);
	globs = parse();
	if(f != stdin)
		fclose(f);

	if(globs->decls){
		fold(globs);
		gf(globs);
	}

	io_fin(gf == gen_asm);

	return 0;
}
