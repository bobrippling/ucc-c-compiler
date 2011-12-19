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

/* TODO */
#define WARN_FORMAT         0
#define WARN_INT_TO_PTR     0
#define WARN_PTR_ARITH      0
#define WARN_SHADOW         0
#define WARN_UNINITIALISED  0
#define WARN_UNUSED_PARAM   0
#define WARN_UNUSED_VAL     0
#define WARN_UNUSED_VAR     0

struct
{
	const char *arg;
	enum warning type;
} arg[] = {
	{ "all",             ~0 },
	{ "extra",            0 }, /* TODO */
	{ "mismatch-arg",    WARN_ARG_MISMATCH                      }, /* x(int); x("hi"); */
	{ "array-comma",     WARN_ARRAY_COMMA                       }, /* { 1, 2, 3, } */
	{ "mismatch-assign", WARN_ASSIGN_MISMATCH                   }, /* int i = &q; */
	{ "return-type",     WARN_RETURN_TYPE                       }, /* void x(){return 5;} and vice versa*/

	{ "sign-compare",    WARN_SIGN_COMPARE                      }, /* unsigned x = 5; if(x == -1)... */
	{ "extern-assume",   WARN_EXTERN_ASSUME                     }, /* int printf(); [^printf]+$ */

	{ "implicit-int",    WARN_IMPLICIT_INT                      }, /* x(){...} and [spec] x; in global/arg scope */
	{ "implicit-func",   WARN_IMPLICIT_FUNC                     }, /* ^main(){x();} */
	{ "implicit",        WARN_IMPLICIT_FUNC | WARN_IMPLICIT_INT }, /* ^main(){x();} */

	/* TODO */
	{ "unused-parameter", WARN_UNUSED_PARAM },
	{ "unused-variable",  WARN_UNUSED_VAR   },
	{ "unused-value",     WARN_UNUSED_VAL   },
	{ "unused",           WARN_UNUSED_PARAM | WARN_UNUSED_VAR | WARN_UNUSED_VAL },

	/* TODO */
	{ "uninitialised",    WARN_UNINITIALISED },

	/* TODO */
	{ "array-bounds",     WARN_UNINITIALISED },

	/* TODO */
	{ "shadow",           WARN_SHADOW }, /* int x; { int x; } */

	/* TODO */
	{ "format",           WARN_FORMAT }, /* int x; { int x; } */

	/* TODO */
	{ "pointer-arith",    WARN_PTR_ARITH }, /* void *x; x++; */
	{ "int-ptr-cast",     WARN_INT_TO_PTR }, /* void *x; x++; */

	{ "pointer-arith",    WARN_INT_TO_PTR }, /* void *x; x++; */
};


FILE *cc_out[NUM_SECTIONS];     /* temporary section files */
char  fnames[NUM_SECTIONS][32]; /* duh */
FILE *cc1_out;                  /* final output */

enum warning warn_mode = ~WARN_VOID_ARITH; /* FIXME */

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

	if(fmt[i-1] == ':'){
		fputc(' ', stderr);
		perror(NULL);
	}else{
		fputc('\n', stderr);
	}

	exit(1);
}

void cc1_warn_at(struct where *where, int die, enum warning w, const char *fmt, ...)
{
	va_list l;

	if(!die && (w & warn_mode) == 0)
		return;

	va_start(l, fmt);
	vwarn(where, fmt, l);
	va_end(l);

	if(die)
		exit(1);
}

void io_cleanup(void)
{
	int i;
	for(i = 0; i < NUM_SECTIONS; i++){
		fclose(cc_out[i]);
		remove(fnames[i]);
	}
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

	atexit(io_cleanup);
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

			if(fprintf(cc1_out, "section .%s\n", section_names[i]) < 0)
				ccdie("write to cc1 output:");

			while(fgets(buf, sizeof buf, cc_out[i]))
				if(fputs(buf, cc1_out) == EOF)
					ccdie("write to cc1 output:");

			if(ferror(cc_out[i]))
				ccdie("read from section file %d:", i);
		}
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

		}else if(!strcmp(argv[i], "-w")){
			warn_mode = WARN_NONE;

		}else if(!strncmp(argv[i], "-W", 2)){
			/*char *arg = argv[i] + 2;*/

			/* TODO: -Wno-... */

			ccdie("%s: -W... not implemented", *argv);

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
